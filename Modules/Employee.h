void employeeMenu(int connectionFD);
int loginEmployee(int connectionFD, int empID, char *password);
void addCustomer(int connectionFD);
void approveRejectLoan(int connectionFD, int empID);
void viewAssignedLoan(int connectionFD, int empID);
int changeEMPPassword(int connectionFD, int empID);

void employeeMenu(int connectionFD)
{
    struct Employee employee;
    int empID;
    int accountNumber, response = 0;
    char password[256];   // make bigger
    int choice;

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "\nEnter Employee ID: ");
    write(connectionFD, writeBuffer, strlen(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    empID = atoi(readBuffer);
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter password: ");
    write(connectionFD, writeBuffer, strlen(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    readBuffer[strcspn(readBuffer, "\r\n")] = '\0';
    strcpy(password, readBuffer);

    if(loginEmployee(connectionFD, empID, password))
    {
        printf("%d logged In\n", empID);
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "\nLogin Successfully^");
        write(connectionFD, writeBuffer, strlen(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));

        while(1)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, EMPMENU);
            write(connectionFD, writeBuffer, strlen(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            choice = atoi(readBuffer);

            printf("Employee choose: %d\n", choice);
            switch(choice)
            {
                case 1: addCustomer(connectionFD); break;
                case 2: modifyCE(connectionFD, 1); break;
                case 3: approveRejectLoan(connectionFD, empID); break;
                case 4: viewAssignedLoan(connectionFD, empID); break;
                case 5:
                    bzero(writeBuffer, sizeof(writeBuffer));
                    strcpy(writeBuffer, "Enter Account Number: ");
                    write(connectionFD, writeBuffer, strlen(writeBuffer));
                    bzero(readBuffer, sizeof(readBuffer));
                    read(connectionFD, readBuffer, sizeof(readBuffer));
                    accountNumber = atoi(readBuffer);
                    transactionHistory(connectionFD, accountNumber);
                    break;
                case 6:
                    response = changeEMPPassword(connectionFD, empID);
                    if(response)
                        strcpy(writeBuffer, "Password changed successfully\nLogin again^");
                    else
                        strcpy(writeBuffer, "Unable to change password^");
                    write(connectionFD, writeBuffer, strlen(writeBuffer));
                    read(connectionFD, readBuffer, sizeof(readBuffer));
                    logout(connectionFD, empID);
                case 7:
                    printf("Employee ID: %d Logged Out!\n", empID);
                    logout(connectionFD, empID);
                    return;
                case 8:
                    printf("Employee ID: %d Exited!\n", empID);
                    exitClient(connectionFD, empID);
                    return;
                default:
                    printf("Invalid choice\n");
            }
        }
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(writeBuffer, "\nInvalid ID or Password^");
        write(connectionFD, writeBuffer, strlen(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        logout(connectionFD, empID);
    }
}


// ================= Login Employee ==================
int loginEmployee(int connectionFD, int empID, char *password)
{
    struct Employee employee;
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if(file == -1)
    {
        perror("Error opening employee file");
        return 0;
    }

    sema = initializeSemaphore(empID);
    setupSignalHandlers();

    if (sem_trywait(sema) == -1) {
        if (errno == EAGAIN)
            printf("Employee %d is already logged in!\n", empID);
        else
            perror("sem_trywait failed");
        close(file);
        return 0;
    }
    lseek(file, 0, SEEK_SET);

    while(read(file, &employee, sizeof(employee)) > 0)
    {
        if (employee.empID == empID &&
            employee.role == 1 &&
            strcmp(employee.password, password) == 0)
        {
            close(file);
            return 1;  //  Login success
        }
    }

    snprintf(semName, sizeof(semName), "/sem_%d", empID);
    sem_t *temp = sem_open(semName, 0);
    if (temp != SEM_FAILED) {
        sem_post(temp);
        sem_close(temp);
    }

    close(file);
    return 0;
}
// void cleanInput(char *buf) {
//     buf[strcspn(buf, "\r\n")] = '\0';
// }

// void flushInput(int connectionFD) {
//     char temp[256];
//     usleep(100000); // small sync delay (100 ms)
//     while (read(connectionFD, temp, sizeof(temp)) > 0) {
//         if (strchr(temp, '\n') || strchr(temp, '^')) break;
//     }
// }

void addCustomer(int connectionFD) {
    struct Customer customer;
    struct trans_histroy th;
    char transactionBuffer[1024];
    #define CUSTOMER 2

    time_t s = time(NULL);
    struct tm *current_time = localtime(&s);

    int file = open(CUSPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (file == -1 || fp == -1) {
        perror("Error opening file");
        return;
    }

    int readBytes;
    // ================== First Name ==================
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter First Name: ");
    write(connectionFD, writeBuffer, strlen(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
    if (readBytes <= 0) {
        printf("Client disconnected during first name entry.\n");
        close(file);
        close(fp);
        return;
    }
    readBuffer[readBytes] = '\0';
    readBuffer[strcspn(readBuffer, "\r\n")] = '\0';
    strncpy(customer.firstName, readBuffer, sizeof(customer.firstName) - 1);
    customer.firstName[sizeof(customer.firstName) - 1] = '\0';

    // ================== Last Name ==================
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Last Name: ");
    write(connectionFD, writeBuffer, strlen(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
    if (readBytes <= 0) {
        printf("Client disconnected during last name entry.\n");
        close(file);
        close(fp);
        return;
    }
    readBuffer[readBytes] = '\0';
    readBuffer[strcspn(readBuffer, "\r\n")] = '\0';
    strncpy(customer.lastName, readBuffer, sizeof(customer.lastName) - 1);
    customer.lastName[sizeof(customer.lastName) - 1] = '\0';

    // ================== Password ==================
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Password: ");
    write(connectionFD, writeBuffer, strlen(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
    if (readBytes <= 0) {
        printf("Client disconnected during password entry.\n");
        close(file);
        close(fp);
        return;
    }
    readBuffer[readBytes] = '\0';
    readBuffer[strcspn(readBuffer, "\r\n")] = '\0';
    strcpy(customer.password, readBuffer);
    // ================== Assign Account Number ==================
    customer.accountNumber = getNextID(CUSTOMER);
    customer.activeStatus = 1;

    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "Assigned Account Number: %d^", customer.accountNumber);
    write(connectionFD, writeBuffer, strlen(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    // ================== Initial Balance ==================
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Opening Balance: ");
    write(connectionFD, writeBuffer, strlen(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
    if (readBytes <= 0) {
        printf("Client disconnected during balance entry.\n");
        close(file);
        close(fp);
        return;
    }
    readBuffer[readBytes] = '\0';
    readBuffer[strcspn(readBuffer, "\r\n")] = '\0';
    customer.balance = atof(readBuffer);

    // ================== Record Opening Transaction ==================
    bzero(transactionBuffer, sizeof(transactionBuffer));
    snprintf(transactionBuffer, sizeof(transactionBuffer),
             "%.2f Opening Balance %02d:%02d:%02d %d-%02d-%02d\n",
             customer.balance,
             current_time->tm_hour, current_time->tm_min, current_time->tm_sec,
             current_time->tm_year + 1900,
             current_time->tm_mon + 1,
             current_time->tm_mday);

    bzero(th.hist, sizeof(th.hist));
    strncpy(th.hist, transactionBuffer, sizeof(th.hist) - 1);
    th.hist[sizeof(th.hist) - 1] = '\0';
    th.acc_no = customer.accountNumber;
    write(fp, &th, sizeof(th));
    close(fp);

    // ================== Save Customer ==================
    lseek(file, 0, SEEK_END);
    write(file, &customer, sizeof(customer));
    close(file);

    printf("Employee added new customer: %s %s (Account: %d, Balance: %.2f\n",customer.firstName, customer.lastName, customer.accountNumber, customer.balance);

    // ================== Notify Client ==================
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Customer added successfully!^");
    write(connectionFD, writeBuffer, strlen(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}


// ================= Approve/Reject Loan =================
void approveRejectLoan(int connectionFD, int empID)
{
    char transactionBuffer[1024];
    struct LoanDetails ld;
    
    struct trans_histroy th;
    time_t s, val = 1;
	struct tm* current_time;
	s = time(NULL);
	current_time = localtime(&s);

    int lID;
    struct Customer cs;

    int file = open(LOANPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND);
    lseek(file, 0, SEEK_SET);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Loan ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    lID = atoi(readBuffer);

    int srcOffset1 = -1, sourceFound1 = 0, srcOffset2 = -1, sourceFound2 = 0;
    while (read(file, &ld, sizeof(ld)) != 0)
    {
        if(ld.loanID == lID)
        {
            srcOffset1 = lseek(file, -sizeof(struct LoanDetails), SEEK_CUR);
            sourceFound1 = 1;
        }
        if(sourceFound1)
            break;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset1, sizeof(struct LoanDetails), getpid()};
    int result1 = fcntl(file, F_SETLK, &fl1);

    int approveFlag = 0, rejectFlag = 0, devactiveFlag = 0;

    if(result1 != -1)
    {
        int file2 = open(CUSPATH, O_CREAT | O_RDWR, 0644);
        while (read(file2, &cs, sizeof(cs)) != 0)
        {
            if(cs.accountNumber == ld.accountNumber)
            {
                srcOffset2 = lseek(file2, -sizeof(struct Customer), SEEK_CUR);
                sourceFound2 = 1;
            }
            if(sourceFound2)
                break;
        }
        struct flock fl2 = {F_WRLCK, SEEK_SET, srcOffset2, sizeof(struct Customer), getpid()};
        int result2 = fcntl(file2, F_SETLKW, &fl2);

        int choice;
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Enter 1 to Approve Loan\nEnter 2 to Reject Loan: ");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        bzero(readBuffer, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        choice = atoi(readBuffer);

        if(choice == 1)
        {
            if(cs.activeStatus == 0)
            {
                ld.status = 3;   // rejected                         
                printf("%d rejected loan for account number: %d reason customer is deactive\n", empID, cs.accountNumber);
                write(file, &ld, sizeof(ld));
                devactiveFlag = 1;
            }
            else
            {
                cs.balance += ld.loanAmount;
                ld.status = 2; // approved

                printf("%d approved loan for account number: %d\n", empID, cs.accountNumber);

                bzero(transactionBuffer, sizeof(transactionBuffer));
                sprintf(transactionBuffer, "%d credited by loan id %d at %02d:%02d:%02d %d-%d-%d\n", ld.loanAmount, lID, current_time->tm_hour, current_time->tm_min,current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);
            
                bzero(th.hist, sizeof(th.hist));
                strcpy(th.hist, transactionBuffer);
                th.acc_no = cs.accountNumber;
                write(fp, &th, sizeof(th));

                write(file2, &cs, sizeof(cs));
                write(file, &ld, sizeof(ld));

                fl2.l_type = F_UNLCK;
                fl2.l_whence = SEEK_SET;
                fl2.l_start = srcOffset2;
                fl2.l_len = sizeof(struct Customer);
                fl2.l_pid = getpid();

                fcntl(file2, F_UNLCK, &fl2);

                close(file2);
                close(fp);
                approveFlag = 1;
            }
        }
        else if(choice == 2)
        {

            printf("%d rejected loan for account number: %d\n", empID, cs.accountNumber);
            ld.status = 3;
            write(file, &ld, sizeof(ld));
            rejectFlag = 1;

            fl2.l_type = F_UNLCK;
            fl2.l_whence = SEEK_SET;
            fl2.l_start = srcOffset2;
            fl2.l_len = sizeof(struct Customer);
            fl2.l_pid = getpid();

            fcntl(file2, F_UNLCK, &fl2);

            close(file2);
        }
        else
        {
            printf("Invalid Choice\n");
        }
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        sprintf(writeBuffer, "Given Loan ID %d is already either approved or rejected^", lID);
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer)); 
    }

    fl1.l_type = F_UNLCK;
    fl1.l_whence = SEEK_SET;
    fl1.l_start = srcOffset1;
    fl1.l_len = sizeof(struct LoanDetails);
    fl1.l_pid = getpid();
    fcntl(file, F_UNLCK, &fl1);

    close(file);

    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));
    if(approveFlag == 1)
    {
        strcat(writeBuffer, "Loan Approved\n");
    }
    else if(rejectFlag == 1)
    {
        strcat(writeBuffer, "Loan rejected\n");
    }
    else if(devactiveFlag == 1)
    {
        strcat(writeBuffer, "Account is already deactivate so can't approve/reject loan\n");
    }
    strcat(writeBuffer, "^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ================= View Assigned Employee =================
void viewAssignedLoan(int connectionFD, int empID)
{
    struct LoanDetails ld;
    int file = open(LOANPATH, O_RDONLY);
    if(file == -1)
    {
        printf("Error in opening file\n");
        return;
    }

    while(read(file, &ld, sizeof(ld)) != 0)
    {
        if(ld.empID == empID && ld.status == 1)
        {
            bzero(readBuffer, sizeof(readBuffer));
            bzero(writeBuffer, sizeof(writeBuffer));
            sprintf(writeBuffer, "Loan ID: %d\nAccount Number: %d\nLoan Amount: %d\n", ld.loanID, ld.accountNumber, ld.loanAmount);
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }
    }
    close(file);
    
    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ================= Change Password =================
int changeEMPPassword(int connectionFD, int empID)
{
    struct Employee emp;
    char newPassword[256];
    int file = open(EMPPATH, O_RDWR);
    if (file == -1) {
        perror("Error opening employee file");
        return 0;
    }

    lseek(file, 0, SEEK_SET);

    int srcOffset = -1, sourceFound = 0;

    // --- Locate employee ---
    while (read(file, &emp, sizeof(emp)) == sizeof(emp))
    {
        if (emp.empID == empID)
        {
            srcOffset = lseek(file, -sizeof(struct Employee), SEEK_CUR);
            sourceFound = 1;
            break;
        }
    }

    if (!sourceFound) {
        printf("Employee ID %d not found!\n", empID);
        close(file);
        return 0;
    }

    // --- Lock record ---
    struct flock fl1 = {0};
    fl1.l_type = F_WRLCK;
    fl1.l_whence = SEEK_SET;
    fl1.l_start = srcOffset;
    fl1.l_len = sizeof(struct Employee);
    fl1.l_pid = getpid();
    fcntl(file, F_SETLKW, &fl1);

    // --- Prompt for new password ---
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter new password: ");
    write(connectionFD, writeBuffer, strlen(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    int bytesRead = read(connectionFD, readBuffer, sizeof(readBuffer));
    if (bytesRead <= 0) {
        perror("Read password failed");
        close(file);
        return 0;
    }

    readBuffer[bytesRead] = '\0';
    readBuffer[strcspn(readBuffer, "\r\n")] = '\0'; // remove newline
    strcpy(newPassword, readBuffer);
    strcpy(emp.password,newPassword);
    lseek(file, srcOffset, SEEK_SET);
    if (write(file, &emp, sizeof(emp)) != sizeof(emp)) {
        perror("Failed to update password record");
        fl1.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl1);
        close(file);
        return 0;
    }

    // --- Unlock ---
    fl1.l_type = F_UNLCK;
    fcntl(file, F_SETLK, &fl1);
    close(file);

    printf("Employee %d changed password successfully\n", empID);
    return 1;
}
