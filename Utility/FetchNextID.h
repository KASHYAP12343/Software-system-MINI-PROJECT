
// File path for persistent counters

// Role identifiers
#define EMPLOYEE 1
#define CUSTOMER 2
#define LOAN 3

int getNextID(int roleType) {
    int file = open(COUNTERPATH, O_RDWR | O_CREAT, 0644);
    if (file == -1) {
        perror("Error opening counter file");
        return -1;
    }

    // Lock entire file for exclusive access
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // lock entire file
    lock.l_pid = getpid();
    fcntl(file, F_SETLKW, &lock);

    // Initialize default values if file is empty
    off_t size = lseek(file, 0, SEEK_END);
    if (size == 0) {
        const char *initData = "EMPLOYEE:1\nCUSTOMER:1000\nLOANID:100";
        write(file, initData, strlen(initData));
        fsync(file);
        lseek(file, 0, SEEK_SET);
    } else {
        lseek(file, 0, SEEK_SET);
    }

    // Read current counters
    char buffer[100];
    int empCounter = 1, cusCounter = 1000, LoanCounter = 100;
    ssize_t bytesRead = read(file, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        sscanf(buffer, "EMPLOYEE:%d\nCUSTOMER:%d\nLOANID:%d", &empCounter, &cusCounter, &LoanCounter);
    }

    int newID = 0;

    // Update counters based on role
    if (roleType == EMPLOYEE) {
        newID = empCounter;
        empCounter++;
    } else if (roleType == CUSTOMER) {
        newID = cusCounter;
        cusCounter++;
    }
    else if (roleType == LOAN){
        newID = LoanCounter;
        LoanCounter++;
    } 
    else {
        printf("Invalid role type!\n");
    }

    // Rewind file and write updated counters
    lseek(file, 0, SEEK_SET);
    ftruncate(file, 0); // clear old content
    dprintf(file, "EMPLOYEE:%d\nCUSTOMER:%d\nLOANID:%d\n", empCounter, cusCounter, LoanCounter);
    fsync(file);

    // Unlock file
    lock.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &lock);
    close(file);

    return newID;
}
