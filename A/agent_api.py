from flask import Flask, request

app = Flask(__name__)

JOB = None

@app.route("/")
def home():
    return "OK"

@app.route("/job")
def job():
    global JOB
    if JOB:
        tmp = JOB
        JOB = None
        return tmp
    return "null"

@app.route("/push", methods=["POST"])
def push():
    global JOB
    JOB = request.data.decode()
    return "ok"

app.run(host="0.0.0.0", port=8080)