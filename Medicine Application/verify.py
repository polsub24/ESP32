import csv

def find_data_by_hash(csv_path, target_hash):
    with open(csv_path, 'r') as file:
        reader = csv.reader(file)
        for row in reader:
            if len(row) != 6:
                continue  # Skip malformed rows
            _, location, machine, temp, hum, hash_val = row
            if hash_val.strip().lower() == target_hash.strip().lower():
                return {
                    "Location": location,
                    "Machine": machine,
                    "Temperature": temp,
                    "Humidity": hum
                }
    return None

def main():
    csv_path = input("Enter path to CSV log file (e.g., logs.csv): ").strip()
    hash_code = input("Enter SHA256 hash to look up: ").strip()

    result = find_data_by_hash(csv_path, hash_code)
    if result:
        print("\n✅ Match found!")
        for key, val in result.items():
            print(f"{key}: {val}")
    else:
        print("\n❌ No matching hash found in the log.")

if __name__ == "__main__":
    main()
