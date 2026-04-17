from flask import Flask, jsonify
import os

app = Flask(__name__)

@app.route('/metrics.json')
def metrics():
    cpu = os.popen("top -bn1 | grep 'Cpu(s)' | awk '{print $2}'").read().strip()
    mem = os.popen("free | grep Mem | awk '{print $3/$2 * 100.0}'").read().strip()

    return jsonify({
        "cpu": float(cpu),
        "memory": float(mem)
    })

app.run(host="0.0.0.0", port=8000)
