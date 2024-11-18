# pairup.c
This program provides a solution to help members in English study group to pair up with each other more efficiently by implementing several algorithms to prioritize the pairing order.

## How to build (Ubuntu 22.04)

- Install the required packages.
```bash
sudo apt-get install build-essential make
```

- Run `make` to build the program.
```bash
make
```

- Run `get-today-google-sheet.sh` to get the latest sheet (`main` will need the csv file it fetched).
```bash
./get-today-google-sheet.sh
```

- The main program `main` we built is located in project root.
```bash
./main
```
