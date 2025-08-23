from flask import Flask, render_template, request, jsonify, send_file
import json
import csv
import os
from datetime import datetime
import time
from flask_socketio import SocketIO, emit
import threading
import serial
import serial.tools.list_ports

app = Flask(__name__)
app.config['SECRET_KEY'] = 'flockyou_secret_key_2024'
socketio = SocketIO(app, cors_allowed_origins="*")

# Global variables
detections = []
gps_data = None
serial_connection = None
gps_enabled = False
flock_device_connected = False
flock_device_port = None
flock_serial_connection = None
oui_database = {}
serial_terminal_socket = None
serial_data_buffer = []
reconnect_attempts = {'flock': 0, 'gps': 0}
max_reconnect_attempts = 5
reconnect_delay = 3  # seconds

# Load OUI database
def load_oui_database():
    """Load the IEEE OUI database for manufacturer lookups"""
    global oui_database
    try:
        with open('oui.txt', 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#') and '(hex)' in line:
                    # Parse OUI line format: "28-6F-B9   (hex)                Nokia Shanghai Bell Co., Ltd."
                    parts = line.split('(hex)')
                    if len(parts) == 2:
                        mac_prefix = parts[0].strip().replace('-', '').replace(' ', '').upper()
                        manufacturer = parts[1].strip()
                        if mac_prefix and manufacturer and len(mac_prefix) == 6:
                            oui_database[mac_prefix] = manufacturer
        print(f"Loaded {len(oui_database)} OUI entries")
    except Exception as e:
        print(f"Error loading OUI database: {e}")

def lookup_manufacturer(mac_address):
    """Look up manufacturer information for a MAC address"""
    if not mac_address:
        return None
    
    # Extract first 6 characters (3 bytes) of MAC address
    mac_clean = mac_address.replace(':', '').replace('-', '').upper()
    if len(mac_clean) >= 6:
        oui = mac_clean[:6]
        return oui_database.get(oui, "Unknown Manufacturer")
    return "Unknown Manufacturer"

# GPS Dongle Configuration
GPS_BAUDRATE = 9600
GPS_TIMEOUT = 1

class GPSData:
    def __init__(self):
        self.latitude = None
        self.longitude = None
        self.altitude = None
        self.timestamp = None
        self.fix_quality = 0
        self.satellites = 0

def parse_nmea_sentence(sentence):
    """Parse NMEA GPS sentence"""
    if not sentence.startswith('$'):
        return None
    
    parts = sentence.strip().split(',')
    if len(parts) < 1:
        return None
    
    sentence_type = parts[0]
    
    if sentence_type == '$GPGGA':  # Global Positioning System Fix Data
        if len(parts) >= 15:
            try:
                time_str = parts[1]
                lat = float(parts[2]) / 100
                lat_dir = parts[3]
                lon = float(parts[4]) / 100
                lon_dir = parts[5]
                fix_quality = int(parts[6])
                satellites = int(parts[7])
                altitude = float(parts[9]) if parts[9] else 0
                
                # Convert to decimal degrees
                if lat_dir == 'S':
                    lat = -lat
                if lon_dir == 'W':
                    lon = -lon
                
                return {
                    'latitude': lat,
                    'longitude': lon,
                    'altitude': altitude,
                    'fix_quality': fix_quality,
                    'satellites': satellites,
                    'timestamp': time_str
                }
            except (ValueError, IndexError):
                return None
    
    return None

def gps_reader():
    """Background thread for reading GPS data"""
    global gps_data, serial_connection, gps_enabled
    
    while gps_enabled:
        if serial_connection and serial_connection.is_open:
            try:
                line = serial_connection.readline().decode('utf-8', errors='ignore')
                if line:
                    parsed = parse_nmea_sentence(line)
                    if parsed:
                        gps_data = parsed
                        socketio.emit('gps_update', parsed)
            except Exception as e:
                print(f"GPS read error: {e}")
                gps_enabled = False
                socketio.emit('gps_disconnected')
                break
        time.sleep(0.1)

def flock_reader():
    """Background thread for reading Flock device data"""
    global flock_serial_connection, flock_device_connected, serial_data_buffer
    
    while flock_device_connected:
        if flock_serial_connection and flock_serial_connection.is_open:
            try:
                line = flock_serial_connection.readline().decode('utf-8', errors='ignore')
                if line:
                    line = line.strip()
                    if line:
                        # Store in buffer for terminal
                        serial_data_buffer.append(line)
                        if len(serial_data_buffer) > 1000:  # Keep last 1000 lines
                            serial_data_buffer.pop(0)
                        
                        # Forward to serial terminal if active
                        if serial_terminal_socket:
                            socketio.emit('serial_data', line, room=serial_terminal_socket)
                        
                        # Try to parse as detection data
                        try:
                            data = json.loads(line)
                            if 'detection_method' in data:
                                # This is a detection, add it
                                add_detection_from_serial(data)
                        except json.JSONDecodeError:
                            # Not JSON, just log it
                            print(f"Flock device: {line}")
                            
            except Exception as e:
                print(f"Flock device read error: {e}")
                flock_device_connected = False
                socketio.emit('flock_disconnected')
                # Trigger reconnection immediately
                attempt_reconnect_flock()
                break
        time.sleep(0.1)

def add_detection_from_serial(data):
    """Add detection from serial data"""
    global detections, gps_data
    
    # Add GPS data if available
    if gps_data and gps_data.get('fix_quality') > 0:
        data['gps'] = {
            'latitude': gps_data.get('latitude'),
            'longitude': gps_data.get('longitude'),
            'altitude': gps_data.get('altitude'),
            'timestamp': gps_data.get('timestamp'),
            'satellites': gps_data.get('satellites')
        }
    
    # Add manufacturer information
    if 'mac_address' in data:
        data['manufacturer'] = lookup_manufacturer(data['mac_address'])
    
    # Add server timestamp
    data['server_timestamp'] = datetime.now().isoformat()
    
    # Add unique ID for aliasing
    data['id'] = len(detections)
    data['alias'] = ''  # Empty alias by default
    
    detections.append(data)
    
    # Emit to connected clients
    socketio.emit('new_detection', data)

def connection_monitor():
    """Background thread for monitoring device connections"""
    global gps_enabled, flock_device_connected, serial_connection, reconnect_attempts
    
    while True:
        # Check GPS connection
        if gps_enabled and (not serial_connection or not serial_connection.is_open):
            gps_enabled = False
            socketio.emit('gps_disconnected')
            print("GPS connection lost")
            # Start reconnection attempts
            attempt_reconnect_gps()
        
        # Check Flock You device connection
        if flock_device_connected:
            try:
                # Test if the connection is still valid
                if not flock_serial_connection or not flock_serial_connection.is_open:
                    flock_device_connected = False
                    socketio.emit('flock_disconnected')
                    print("Flock You device connection lost")
                    # Start reconnection attempts
                    attempt_reconnect_flock()
                else:
                    # Try a simple read to test connection
                    flock_serial_connection.in_waiting
            except Exception as e:
                print(f"Flock device connection test failed: {e}")
                flock_device_connected = False
                socketio.emit('flock_disconnected')
                # Start reconnection attempts
                attempt_reconnect_flock()
        
        time.sleep(2)  # Check every 2 seconds

def attempt_reconnect_flock():
    """Attempt to reconnect to Flock device"""
    global flock_device_connected, reconnect_attempts, flock_serial_connection
    
    def reconnect_thread():
        global flock_device_connected, reconnect_attempts, flock_serial_connection
        
        while not flock_device_connected and reconnect_attempts['flock'] < max_reconnect_attempts:
            try:
                # Try to reconnect
                if flock_serial_connection:
                    try:
                        flock_serial_connection.close()
                    except:
                        pass
                
                # Wait a moment for the device to be ready
                time.sleep(1)
                
                flock_serial_connection = serial.Serial(flock_device_port, 115200, timeout=1)
                
                # Test the connection
                test_data = flock_serial_connection.readline()
                
                # If successful, update status
                flock_device_connected = True
                reconnect_attempts['flock'] = 0
                print(f"Successfully reconnected to Flock device on {flock_device_port}")
                socketio.emit('flock_reconnected', {'port': flock_device_port})
                
                # Restart the reading thread
                flock_thread = threading.Thread(target=flock_reader, daemon=True)
                flock_thread.start()
                return
                
            except Exception as e:
                reconnect_attempts['flock'] += 1
                time.sleep(reconnect_delay)
        
        if reconnect_attempts['flock'] >= max_reconnect_attempts:
            print("Max reconnection attempts reached for Flock device")
            socketio.emit('reconnect_failed', {'device': 'flock'})
            reconnect_attempts['flock'] = 0  # Reset for future attempts
    
    thread = threading.Thread(target=reconnect_thread, daemon=True)
    thread.start()

def attempt_reconnect_gps():
    """Attempt to reconnect to GPS device"""
    global gps_enabled, reconnect_attempts
    
    def reconnect_thread():
        global gps_enabled, reconnect_attempts
        
        while not gps_enabled and reconnect_attempts['gps'] < max_reconnect_attempts:
            try:
                # Try to reconnect
                test_ser = serial.Serial(serial_connection.port, GPS_BAUDRATE, timeout=1)
                test_ser.close()
                
                # If successful, update status
                gps_enabled = True
                reconnect_attempts['gps'] = 0
                print(f"Successfully reconnected to GPS device on {serial_connection.port}")
                socketio.emit('gps_reconnected', {'port': serial_connection.port})
                return
                
            except Exception as e:
                reconnect_attempts['gps'] += 1
                time.sleep(reconnect_delay)
        
        if reconnect_attempts['gps'] >= max_reconnect_attempts:
            print("Max reconnection attempts reached for GPS device")
            socketio.emit('reconnect_failed', {'device': 'gps'})
    
    thread = threading.Thread(target=reconnect_thread, daemon=True)
    thread.start()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/detections', methods=['GET'])
def get_detections():
    """Get all detections with optional filtering"""
    filter_type = request.args.get('filter', 'all')
    
    if filter_type == 'all':
        return jsonify(detections)
    else:
        filtered = [d for d in detections if d.get('detection_method') == filter_type]
        return jsonify(filtered)

@app.route('/api/detections', methods=['POST'])
def add_detection():
    """Add a new detection from serial data"""
    global detections, gps_data
    
    data = request.json
    
    # Add GPS data if available
    if gps_data and gps_data.get('fix_quality') > 0:
        data['gps'] = {
            'latitude': gps_data.get('latitude'),
            'longitude': gps_data.get('longitude'),
            'altitude': gps_data.get('altitude'),
            'timestamp': gps_data.get('timestamp'),
            'satellites': gps_data.get('satellites')
        }
    
    # Add manufacturer information
    if 'mac_address' in data:
        data['manufacturer'] = lookup_manufacturer(data['mac_address'])
    
    # Add server timestamp
    data['server_timestamp'] = datetime.now().isoformat()
    
    detections.append(data)
    
    # Emit to connected clients
    socketio.emit('new_detection', data)
    
    return jsonify({'status': 'success', 'id': len(detections)})

@app.route('/api/gps/connect', methods=['POST'])
def connect_gps():
    """Connect to GPS dongle"""
    global serial_connection, gps_enabled
    
    data = request.json
    port = data.get('port')
    
    try:
        if serial_connection:
            serial_connection.close()
        
        serial_connection = serial.Serial(port, GPS_BAUDRATE, timeout=GPS_TIMEOUT)
        gps_enabled = True
        
        # Start GPS reading thread
        gps_thread = threading.Thread(target=gps_reader, daemon=True)
        gps_thread.start()
        
        return jsonify({'status': 'success', 'message': f'Connected to {port}'})
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 400

@app.route('/api/gps/disconnect', methods=['POST'])
def disconnect_gps():
    """Disconnect GPS dongle"""
    global serial_connection, gps_enabled
    
    gps_enabled = False
    if serial_connection:
        serial_connection.close()
        serial_connection = None
    
    return jsonify({'status': 'success', 'message': 'GPS disconnected'})

@app.route('/api/flock/connect', methods=['POST'])
def connect_flock():
    """Connect to Flock You device"""
    global flock_device_connected, flock_device_port, flock_serial_connection
    
    data = request.json
    port = data.get('port')
    
    try:
        # Create persistent connection to the port
        flock_serial_connection = serial.Serial(port, 115200, timeout=1)
        flock_device_connected = True
        flock_device_port = port
        
        # Start reading thread
        flock_thread = threading.Thread(target=flock_reader, daemon=True)
        flock_thread.start()
        
        return jsonify({'status': 'success', 'message': f'Connected to Flock You device on {port}'})
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 400

@app.route('/api/flock/disconnect', methods=['POST'])
def disconnect_flock():
    """Disconnect Flock You device"""
    global flock_device_connected, flock_device_port, flock_serial_connection
    
    flock_device_connected = False
    flock_device_port = None
    
    if flock_serial_connection and flock_serial_connection.is_open:
        flock_serial_connection.close()
        flock_serial_connection = None
    
    return jsonify({'status': 'success', 'message': 'Flock You device disconnected'})

@app.route('/api/status', methods=['GET'])
def get_status():
    """Get connection status of both devices"""
    return jsonify({
        'gps_connected': gps_enabled,
        'gps_port': serial_connection.port if serial_connection else None,
        'flock_connected': flock_device_connected,
        'flock_port': flock_device_port
    })

@app.route('/api/gps/ports', methods=['GET'])
def get_gps_ports():
    """Get available serial ports for GPS"""
    ports = []
    for port in serial.tools.list_ports.comports():
        port_info = {
            'device': port.device,
            'description': port.description,
            'manufacturer': port.manufacturer if port.manufacturer else 'Unknown',
            'product': port.product if port.product else 'Unknown',
            'vid': port.vid,
            'pid': port.pid
        }
        ports.append(port_info)
    return jsonify(ports)

@app.route('/api/flock/ports', methods=['GET'])
def get_flock_ports():
    """Get available serial ports for Flock You device"""
    ports = []
    for port in serial.tools.list_ports.comports():
        port_info = {
            'device': port.device,
            'description': port.description,
            'manufacturer': port.manufacturer if port.manufacturer else 'Unknown',
            'product': port.product if port.product else 'Unknown',
            'vid': port.vid,
            'pid': port.pid
        }
        ports.append(port_info)
    return jsonify(ports)

@app.route('/api/export/csv', methods=['GET'])
def export_csv():
    """Export detections as CSV"""
    if not detections:
        return jsonify({'status': 'error', 'message': 'No detections to export'}), 400
    
    filename = f"flockyou_detections_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    filepath = os.path.join('exports', filename)
    
    os.makedirs('exports', exist_ok=True)
    
    with open(filepath, 'w', newline='', encoding='utf-8') as csvfile:
        fieldnames = [
            'timestamp', 'detection_time', 'protocol', 'detection_method',
            'ssid', 'mac_address', 'manufacturer', 'alias', 'rssi', 'signal_strength', 'channel',
            'latitude', 'longitude', 'altitude', 'gps_timestamp', 'satellites'
        ]
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        
        for detection in detections:
            row = {
                'timestamp': detection.get('timestamp'),
                'detection_time': detection.get('detection_time'),
                'protocol': detection.get('protocol'),
                'detection_method': detection.get('detection_method'),
                'ssid': detection.get('ssid', ''),
                'mac_address': detection.get('mac_address'),
                'manufacturer': detection.get('manufacturer', 'Unknown'),
                'alias': detection.get('alias', ''),
                'rssi': detection.get('rssi'),
                'signal_strength': detection.get('signal_strength'),
                'channel': detection.get('channel'),
                'latitude': detection.get('gps', {}).get('latitude'),
                'longitude': detection.get('gps', {}).get('longitude'),
                'altitude': detection.get('gps', {}).get('altitude'),
                'gps_timestamp': detection.get('gps', {}).get('timestamp'),
                'satellites': detection.get('gps', {}).get('satellites')
            }
            writer.writerow(row)
    
    return send_file(filepath, as_attachment=True, download_name=filename)

@app.route('/api/export/kml', methods=['GET'])
def export_kml():
    """Export detections as KML"""
    if not detections:
        return jsonify({'status': 'error', 'message': 'No detections to export'}), 400
    
    filename = f"flockyou_detections_{datetime.now().strftime('%Y%m%d_%H%M%S')}.kml"
    filepath = os.path.join('exports', filename)
    
    os.makedirs('exports', exist_ok=True)
    
    kml_content = f"""<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
<Document>
    <name>Flock You Detections</name>
    <description>Surveillance device detections with GPS coordinates</description>
"""
    
    for i, detection in enumerate(detections):
        gps = detection.get('gps', {})
        if gps.get('latitude') and gps.get('longitude'):
            kml_content += f"""
    <Placemark>
        <name>Detection {i+1}</name>
        <description>
            <![CDATA[
            <b>Protocol:</b> {detection.get('protocol')}<br/>
            <b>Method:</b> {detection.get('detection_method')}<br/>
            <b>SSID:</b> {detection.get('ssid', 'N/A')}<br/>
            <b>MAC:</b> {detection.get('mac_address')}<br/>
            <b>Manufacturer:</b> {detection.get('manufacturer', 'Unknown')}<br/>
            <b>Alias:</b> {detection.get('alias', 'N/A')}<br/>
            <b>RSSI:</b> {detection.get('rssi')} dBm<br/>
            <b>Signal:</b> {detection.get('signal_strength')}<br/>
            <b>Channel:</b> {detection.get('channel')}<br/>
            <b>Time:</b> {detection.get('detection_time')}<br/>
            <b>GPS Satellites:</b> {gps.get('satellites', 'N/A')}
            ]]>
        </description>
        <Point>
            <coordinates>{gps.get('longitude')},{gps.get('latitude')},{gps.get('altitude', 0)}</coordinates>
        </Point>
    </Placemark>
"""
    
    kml_content += """
</Document>
</kml>"""
    
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(kml_content)
    
    return send_file(filepath, as_attachment=True, download_name=filename)

@app.route('/api/clear', methods=['POST'])
def clear_detections():
    """Clear all detections"""
    global detections
    detections.clear()
    socketio.emit('detections_cleared')
    return jsonify({'status': 'success', 'message': 'All detections cleared'})

@app.route('/api/detection/alias', methods=['POST'])
def update_detection_alias():
    """Update detection alias"""
    global detections
    
    data = request.json
    detection_id = data.get('id')
    alias = data.get('alias', '').strip()
    
    if detection_id is None:
        return jsonify({'status': 'error', 'message': 'Detection ID required'}), 400
    
    # Find and update the detection
    for detection in detections:
        if detection.get('id') == detection_id:
            detection['alias'] = alias
            # Emit update to all clients
            socketio.emit('detection_updated', detection)
            return jsonify({'status': 'success', 'message': 'Alias updated'})
    
    return jsonify({'status': 'error', 'message': 'Detection not found'}), 404

@app.route('/api/oui/search', methods=['POST'])
def search_oui():
    """Search OUI database"""
    global oui_database
    
    data = request.json
    query = data.get('query', '').strip()
    
    if not query:
        return jsonify({'status': 'error', 'message': 'Query required'}), 400
    
    results = []
    
    # Clean the query - remove colons and spaces, convert to uppercase
    clean_query = query.replace(':', '').replace(' ', '').upper()
    
    # Check if query looks like a MAC address (6 hex characters)
    if len(clean_query) >= 6 and all(c in '0123456789ABCDEF' for c in clean_query[:6]):
        # Search by MAC prefix
        mac_prefix = clean_query[:6]
        if mac_prefix in oui_database:
            results.append({
                'mac': mac_prefix,
                'manufacturer': oui_database[mac_prefix]
            })
    else:
        # Search by manufacturer name
        query_lower = query.lower()
        for mac, manufacturer in oui_database.items():
            if query_lower in manufacturer.lower():
                results.append({
                    'mac': mac,
                    'manufacturer': manufacturer
                })
                if len(results) >= 100:  # Increased limit
                    break
    
    print(f"Search query: '{query}' -> '{clean_query}', found {len(results)} results")
    
    return jsonify({
        'status': 'success',
        'results': results,
        'count': len(results)
    })

@app.route('/api/oui/all')
def get_all_oui():
    """Get all OUI entries"""
    global oui_database
    
    # Return all entries
    results = []
    for mac, manufacturer in oui_database.items():
        results.append({
            'mac': mac,
            'manufacturer': manufacturer
        })
    
    return jsonify({
        'status': 'success',
        'results': results,
        'count': len(results),
        'total': len(oui_database)
    })

@app.route('/api/oui/refresh', methods=['POST'])
def refresh_oui_database():
    """Refresh OUI database from IEEE website"""
    global oui_database
    
    try:
        import urllib.request
        import tempfile
        import os
        
        # Download the latest OUI database
        url = "https://standards-oui.ieee.org/oui/oui.txt"
        print(f"Downloading OUI database from {url}...")
        
        # Create a temporary file
        with tempfile.NamedTemporaryFile(delete=False, suffix='.txt') as temp_file:
            temp_path = temp_file.name
        
        # Download the file
        urllib.request.urlretrieve(url, temp_path)
        
        # Parse the downloaded file
        new_oui_database = {}
        with open(temp_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#') and '\t' in line:
                    parts = line.split('\t')
                    if len(parts) >= 3:
                        mac = parts[0].strip().replace('-', '').upper()
                        manufacturer = parts[2].strip()
                        if len(mac) == 6 and manufacturer:
                            new_oui_database[mac] = manufacturer
        
        # Clean up temporary file
        os.unlink(temp_path)
        
        # Update the global database
        oui_database = new_oui_database
        
        # Save to local file
        with open('oui.txt', 'w', encoding='utf-8') as f:
            for mac, manufacturer in oui_database.items():
                f.write(f"{mac}\t{manufacturer}\n")
        
        print(f"Successfully refreshed OUI database with {len(oui_database)} entries")
        
        return jsonify({
            'status': 'success',
            'message': 'Database refreshed successfully',
            'count': len(oui_database)
        })
        
    except Exception as e:
        print(f"Error refreshing OUI database: {str(e)}")
        return jsonify({
            'status': 'error',
            'message': f'Failed to refresh database: {str(e)}'
        }), 500

# Socket.IO event handlers
@socketio.on('connect')
def handle_connect():
    print(f"Client connected: {request.sid}")

@socketio.on('disconnect')
def handle_disconnect():
    print(f"Client disconnected: {request.sid}")

@socketio.on('request_serial_terminal')
def handle_serial_terminal_request(data):
    """Handle serial terminal connection request"""
    global serial_terminal_socket, serial_data_buffer
    port = data.get('port')
    
    if not port:
        emit('serial_error', {'message': 'No port specified'})
        return
    
    if not flock_device_connected or flock_device_port != port:
        emit('serial_error', {'message': 'Device not connected. Please connect to the Sniffer device first.'})
        return
    
    try:
        serial_terminal_socket = request.sid
        emit('serial_connected')
        
        # Send recent buffer data
        for line in serial_data_buffer[-50:]:  # Send last 50 lines
            socketio.emit('serial_data', line, room=serial_terminal_socket)
        
    except Exception as e:
        emit('serial_error', {'message': f'Failed to start terminal: {str(e)}'})

if __name__ == '__main__':
    # Load OUI database on startup
    load_oui_database()
    
    # Start connection monitor thread
    monitor_thread = threading.Thread(target=connection_monitor, daemon=True)
    monitor_thread.start()
    
    socketio.run(app, debug=True, host='0.0.0.0', port=5001)
