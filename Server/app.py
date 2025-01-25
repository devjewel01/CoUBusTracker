from flask import Flask, request, jsonify, Response, send_file
from flask_cors import CORS
import sqlite3
from datetime import datetime
import os

app = Flask(__name__)
CORS(app)

# Database setup - using absolute path
DB_PATH = '/home/roboict/cou_bus/bus_tracking.db'

def init_db():
    print("Initializing database...")  # Debug log
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.cursor()
        # Create GPS data table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS gps_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                latitude REAL,
                longitude REAL,
                altitude REAL,
                speed REAL,
                satellites INTEGER,
                timestamp TEXT
            )
        ''')
        # Create table for latest frame
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS video_frames (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                frame BLOB,
                timestamp TEXT
            )
        ''')
        conn.commit()
    print("Database initialization complete!")  # Debug log

# Make sure database directory exists
os.makedirs(os.path.dirname(DB_PATH), exist_ok=True)

# Initialize database at startup
init_db()

@app.route('/api/gps', methods=['POST', 'GET'])
def handle_gps():
    if request.method == 'POST':
        try:
            data = request.json

            # Insert GPS data into database
            with sqlite3.connect(DB_PATH) as conn:
                cursor = conn.cursor()
                cursor.execute('''
                    INSERT INTO gps_data (latitude, longitude, altitude, speed, satellites, timestamp)
                    VALUES (?, ?, ?, ?, ?, ?)
                ''', (
                    data['lat'],
                    data['lng'],
                    data['alt'],
                    data['speed'],
                    data['satellites'],
                    datetime.now().isoformat()
                ))
                conn.commit()

            return jsonify({"status": "success"}), 200
        except Exception as e:
            return jsonify({"error": str(e)}), 500
    else:  # GET request
        try:
            with sqlite3.connect(DB_PATH) as conn:
                cursor = conn.cursor()
                cursor.execute('''
                    SELECT latitude, longitude, altitude, speed, satellites, timestamp
                    FROM gps_data
                    ORDER BY id DESC
                    LIMIT 1
                ''')

                result = cursor.fetchone()
                if result:
                    return jsonify({
                        "lat": result[0],
                        "lng": result[1],
                        "alt": result[2],
                        "speed": result[3],
                        "satellites": result[4],
                        "timestamp": result[5]
                    }), 200
                else:
                    return jsonify({"error": "No GPS data available"}), 404
        except Exception as e:
            return jsonify({"error": str(e)}), 500

@app.route('/api/stream', methods=['POST', 'GET'])
def handle_stream():
    if request.method == 'POST':
        try:
            # Get the frame data from request
            frame_data = request.data

            # Store frame in database
            with sqlite3.connect(DB_PATH) as conn:
                cursor = conn.cursor()
                cursor.execute('''
                    INSERT INTO video_frames (frame, timestamp)
                    VALUES (?, ?)
                ''', (frame_data, datetime.now().isoformat()))
                conn.commit()

            return jsonify({"status": "success"}), 200
        except Exception as e:
            return jsonify({"error": str(e)}), 500
    else:  # GET request
        try:
            with sqlite3.connect(DB_PATH) as conn:
                cursor = conn.cursor()
                cursor.execute('''
                    SELECT frame
                    FROM video_frames
                    ORDER BY id DESC
                    LIMIT 1
                ''')
                result = cursor.fetchone()

                if result:
                    return Response(result[0], mimetype='image/jpeg')
                else:
                    return jsonify({"error": "No frame available"}), 404
        except Exception as e:
            return jsonify({"error": str(e)}), 500

@app.route('/')
def index():
    return send_file('index.html')

if __name__ == '__main__':
    # Run the server on port 5000
    app.run(host='0.0.0.0', port=5000)