#!/usr/bin/env python3

import json

def main():
    times: dict[str, tuple[int, int]] = {}

    try:
        print("Loading JSON")

        with open("profile.json", "r") as f:
            data = json.loads(f.read());

        for i, entry in enumerate(data):
            print(f"Processing line: {i + 1}", end="")
            print("\r", end="")

            if entry["name"] not in times.keys():
                times[entry["name"]] = (int(entry["dur"]), 1)
            else:
                times[entry["name"]] = (times[entry["name"]][0] + int(entry["dur"]), times[entry["name"]][1] + 1)

        print("Sorting items")

        times = { k: v for k, v in sorted(times.items(), key=lambda item: item[1]) }

        print("Done")

        print()

        print(f"| {"Function Name":50} | {"Total Duration (ms)":20} | {"Invocation Count":20} | {"Average Duration (ms)":20} |");
        for key, value in reversed(times.items()):
            print(f"| {key:50} | {value[0] / 1000:20} | {value[1]:20} | {value[0] / (value[1] * 1000):20.3f} |")
        
    except Exception as e:
        print("Failed to read profiling data from 'profile.json'")
        print(str(e))

if __name__ == "__main__":
    main()
