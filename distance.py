from flask import Flask, render_template
import sqlite3

app = Flask(__name__)
DB_NAME = "distance.db"

def get_data():
    conn = sqlite3.connect(DB_NAME)
    cur = conn.cursor()
    cur.execute("""
        SELECT timestamp, distance
        FROM distance_log
        ORDER BY id DESC
        LIMIT 50
    """)
    rows = cur.fetchall()
    conn.close()
    return rows

@app.route("/")
def index():
    data = get_data()
    return render_template("index.html", data=data)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)