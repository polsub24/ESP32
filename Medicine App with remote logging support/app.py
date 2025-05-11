from flask import Flask, request, render_template
import requests

app = Flask(__name__)

# Google Apps Script Web App URL
WEB_APP_URL = "https://script.google.com/macros/s/AKfycbzgpYu25WFiYbWmOuIV5fFMmciDjg5n_3aYd7SS0fu_R-XPqLm9nv5USEJjW8loiPHu/exec"

def fetch_sheet_data():
    try:
        response = requests.get(WEB_APP_URL)
        if response.status_code == 200:
            data = response.json()
            return data
        else:
            print("Failed to fetch data from Google Sheets Web App")
            return []
    except Exception as e:
        print(f"Error fetching data: {e}")
        return []

@app.route('/', methods=['GET', 'POST'])
def index():
    match_found = None

    if request.method == 'POST':
        location = request.form['location'].strip()
        machine = request.form['machine'].strip()
        temperature = float(request.form['temperature'])
        humidity = int(request.form['humidity'])

        sheet_data = fetch_sheet_data()

        for row in sheet_data:
            if len(row) >= 6:
                _, loc, mach, temp, hum, _ = row
                if (loc == location and mach == machine and 
                    float(temp) == temperature and int(hum) == humidity):
                    match_found = True
                    break
        else:
            match_found = False

    return render_template('index.html', match_found=match_found)

if __name__ == '__main__':
    app.run(debug=True)
