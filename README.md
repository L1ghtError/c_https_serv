#Simple HTTP server

To build executable, run:
$ make
To clean binary directory, run:
```bash
$ make clean
```
to run server, execute the command below:
```bash
$ cd ./bin/ && ./http_server && cd ..
```
for ports that lower than 1024, execute:
```bash
$ cd ./bin/ && sudo ./http_server && cd ..
```
