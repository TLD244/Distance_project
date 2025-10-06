#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sqlite3.h>
#include <time.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define DB_NAME "distance.db"

int main() {
    int fd; // file descriptor
    struct termios tty;
    char line[128];        // assemble complete line
    int pos = 0;           // current position in line[]
    sqlite3 *db;
    char *err_msg = 0;

    //Open serial port
    fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
    if(fd < 0) { perror("Serial open"); return 1; }
    
    // Configure port for serial protocol
    tcgetattr(fd, &tty);
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tcsetattr(fd, TCSANOW, &tty);

    // Open SQLite DB
    if(sqlite3_open(DB_NAME, &db)) {
        fprintf(stderr, "Can't open DB : %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    // Create table if not exists
    const char *sql = "CREATE TABLE IF NOT EXISTS distance_log("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                      "distance REAL);";

    if(sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr,"SQL error: %s\n", err_msg);
        sqlite3_free(err_msg); 
    }

    printf("Logging data... Press Ctrl+C to stop.\n");

    char buf[64];   // temporary serial buffer

    while (1) {
        int n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                char c = buf[i];

                if (c == '\n') {
                    line[pos] = '\0';   // terminate string
                    printf("RAW: [%s]\n", line);

                    if (strncmp(line, "Distance:", 9) == 0) {
                        float distance = atof(line + 9);

                        char query[128];
                        snprintf(query, sizeof(query),
                                 "INSERT INTO distance_log(distance) VALUES(%.2f);",
                                 distance);

                        if (sqlite3_exec(db, query, 0, 0, &err_msg) != SQLITE_OK) {
                            fprintf(stderr, "SQL insert error: %s\n", err_msg);
                            sqlite3_free(err_msg);
                        } else {
                            printf("Logged: %.2f cm\n", distance);
                        }
                    }

                    pos = 0;  // reset line buffer

                } else if (c != '\r') {
                    if (pos < (int)sizeof(line) - 1) {
                        line[pos++] = c;
                    }
                }
            }
        }

        usleep(200000); // 0.2s pause
    }

    return 0;
}
