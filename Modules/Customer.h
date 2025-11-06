void customerMenu(int connectionFD);
int loginCustomer(int connectionFD, int accountNumber, char *password);
void withdrawMoney(int connectionFD, int accountNumber);
void depositMoney(int connectionFD, int accountNumber);
void customerBal(int connectionFD, int accountNumber);
void applyLoan(int connectionFD, int accountNumber);
void transferFunds(int connectionFD, int accountNumber, int destAcc, float amt);
void addFeedback(int connectionFD,int accountNumber);
void transactionHistory(int connectionFD, int accountNumber);
int changePassword(int connectionFD, int accountNumber);
void logout(int connectionFD, int id);

int writeBytes, readBytes, key, loginOffset;
char readBuffer[4096], writeBuffer[4096];

void customerMenu(int connectionFD){
    struct Customer newCustomer;
    int accountNumber;
    int destAcc;
    int response = 0;
    char password[20];
    char newPassword[20];
    float amount;

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "\nEnter account number: ");
    writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
    if(writeBytes == -1)
    {
        printf("Unable to send data\n");
    }
    else
    {
        // read account number
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
        if(readBytes == -1)
        {
            printf("Unable to read data\n");
        }
        else
        {
            accountNumber = atoi(readBuffer);
        }
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer,  "Enter password: ");
    writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
    if(writeBytes == -1)
    {
        printf("Unable to send data\n");
    }
    else
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
        strcpy(password, readBuffer);

        if (loginCustomer(connectionFD, accountNumber, password))
        {            
            while(1)
            {   
                bzero(writeBuffer, sizeof(writeBuffer));
                strcpy(writeBuffer, CUSMENU);
                writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
                if(writeBytes == -1)
                {
                    printf("Unable to write to client\n");
                }
                else
                {
                    printf("Menu sent to the client\n");
                    bzero(readBuffer, sizeof(readBuffer));
                    int readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
                    if(readBytes == -1)
                    {
                        printf("Unable to read client data\n");
                    }
                    else
                    {
                        int choice = atoi(readBuffer);
                        printf("Customer entered: %d\n", choice);

                        switch(choice)
                        {
                            case 1:
                                depositMoney(connectionFD, accountNumber);
                                break;
                            case 2:
                                withdrawMoney(connectionFD, accountNumber);
                                break;
                            case 3:
                                customerBal(connectionFD, accountNumber);;
                                break;
                            case 4:
                                applyLoan(connectionFD, accountNumber);
                                break;
                            case 5:
                                bzero(writeBuffer, sizeof(writeBuffer));
                                strcpy(writeBuffer, "Enter dest account number: ");
                                write(connectionFD, writeBuffer, sizeof(writeBuffer));

                                bzero(readBuffer, sizeof(readBuffer));
                                read(connectionFD, readBuffer, sizeof(readBuffer));
                                destAcc = atoi(readBuffer);
                                
                                float amt;
                                bzero(writeBuffer, sizeof(writeBuffer));
                                strcpy(writeBuffer, "Enter amount: ");
                                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                                bzero(readBuffer, sizeof(readBuffer));
                                read(connectionFD, readBuffer, sizeof(readBuffer));

                                amt = atof(readBuffer);
                                transferFunds(connectionFD, accountNumber, destAcc, amt);
                                break;
                            case 6:
                                response = changePassword(connectionFD, accountNumber);
                                if(!response)
                                {
                                    bzero(writeBuffer, sizeof(writeBuffer));
                                    strcpy(writeBuffer, "Unable to change password^");
                                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                                    read(connectionFD, readBuffer, sizeof(readBuffer));
                                }
                                else
                                { 
                                    bzero(writeBuffer, sizeof(writeBuffer));
                                    strcpy(writeBuffer, "Password changed successfully^");
                                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                                    read(connectionFD, readBuffer, sizeof(readBuffer)); 
                                    logout(connectionFD, accountNumber);

                                }                                    
                            case 7:
                                // View Transaction
                                transactionHistory(connectionFD, accountNumber);
                                break;
                            case 8:
                                // Add Feedback
                                addFeedback(connectionFD,accountNumber);
                                break;
                            case 9:
                                // Logout
                                printf("%d logged out!\n", accountNumber);
                                logout(connectionFD, accountNumber);
                                return;
                            default:
                                write(connectionFD, "Invalid Choice from customer menu\n", sizeof("Invalid Choice from customer menu\n"));                                
                        }
                    }
                }
            }
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            bzero(readBuffer, sizeof(readBuffer));
            strcpy(writeBuffer, "\nInvalid ID or Password^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            logout(connectionFD, accountNumber);
        }
    }
}

// ======================= Login system =======================
int loginCustomer(int connectionFD, int accountNumber, char *password) {
    struct Customer customer;
    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);

    if (file == -1) {
        printf("Error opening file!\n");
        return 0;
    }

    sema = initializeSemaphore(accountNumber);

    setupSignalHandlers();

    if (sem_trywait(sema) == -1) {
        if (errno == EAGAIN) {
            printf("Customer with account number %d is already logged in!\n", accountNumber);
        } else {
            perror("sem_trywait failed");
        }
        close(file);
        return 0;
    }

    lseek(file, 0, SEEK_SET);
    while(read(file, &customer, sizeof(customer)) != 0)
    {
        if (customer.accountNumber == accountNumber && strcmp(customer.password,password) == 0 && customer.activeStatus == 1) {
            printf("Customer whose acc no.: %d loggedIn\n", accountNumber);
            close(file);
            return 1;
        }
    }

    // sem_post(sema);

    snprintf(semName, 50, "/sem_%d", accountNumber);

    sem_t *sema = sem_open(semName, 0);
    if (sema != SEM_FAILED) {
        sem_post(sema);
        sem_close(sema); 
        sem_unlink(semName);    
    }

    close(file);
    return 0;
}

// ======================= Deposit Money =======================
void depositMoney(int connectionFD, int accountNumber){
    char readBuffer[4096], writeBuffer[4096], transactionBuffer[1024];

    struct Customer customer;
    struct trans_histroy th;

    time_t s, val = 1;
	struct tm* current_time;
	s = time(NULL);
	current_time = localtime(&s);

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    lseek(fp, 0, SEEK_END);
  
    int found = 0;
    float depositAmount;

    if (file == -1) {
        printf("Error opening file!\n");
        return;
    }

    while(read(file, &customer, sizeof(customer)) != 0) {
        if (customer.accountNumber == accountNumber) {
            break;
        }
    }
    int offset = lseek(file, -sizeof(struct Customer), SEEK_CUR);

    struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
    int lockStatus = fcntl(file, F_SETLKW, &fl);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter the amount to deposit: ");
    writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));

    if(writeBytes == -1)
    {
        printf("Unable to write client\n");
    }
    else
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
        if(readBytes == -1)
        {
            printf("Unable to read client data\n");
        }
        else
        {
            depositAmount = atof(readBuffer);
            printf("Customer whose acc no.: %d deposited %.2f\n", accountNumber, depositAmount);

            lseek(file, 0, SEEK_SET);
            while(read(file, &customer, sizeof(customer)) != 0) {
                if (customer.accountNumber == accountNumber) {
                    break;
                }
            }
            lseek(file, -sizeof(struct Customer), SEEK_CUR);

            customer.balance += depositAmount;
            
            bzero(transactionBuffer, sizeof(transactionBuffer));
            sprintf(transactionBuffer, "%.2f deposited at %02d:%02d:%02d %d-%d-%d\n", depositAmount, current_time->tm_hour, current_time->tm_min,current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);
            
            bzero(th.hist, sizeof(th.hist));
            strcpy(th.hist, transactionBuffer);
            th.acc_no = customer.accountNumber;
            write(fp, &th, sizeof(th));
            write(file, &customer, sizeof(customer));


            fl.l_type = F_UNLCK;
            fcntl(file, F_SETLK, &fl);
            
            close(fp);
            close(file);

            bzero(readBuffer, sizeof(readBuffer));
            bzero(writeBuffer, sizeof(writeBuffer));
            sprintf(writeBuffer, "Deposit successful!^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }
    }
    return;
}

// ======================= View Balance =======================
void customerBal(int connectionFD, int accountNumber){
    char readBuffer[4096], writeBuffer[4096];
    struct Customer customer;
    int file = open(CUSPATH, O_RDONLY);
    if (file == -1) {
        printf("Error opening file!\n");
        return ;
    }
    float updatedBalance = 0;

    lseek(file, 0, SEEK_SET);
    while(read(file, &customer, sizeof(customer)) != 0)
    {
        if (customer.accountNumber == accountNumber) {
            updatedBalance = customer.balance;
            break;
        }
    }
    close(file);
    
    printf("Current balance of %d: %.2f\n", accountNumber, updatedBalance);
    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "The current balance is: %.2f^", updatedBalance);
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));

    return;
}

// ======================= Withdraw Money =======================
void withdrawMoney(int connectionFD, int accountNumber){
    char readBuffer[4096], writeBuffer[4096], transactionBuffer[1024];
    struct Customer customer;
    struct trans_histroy th;

    time_t s, val = 1;
	struct tm* current_time;
	s = time(NULL);
	current_time = localtime(&s);

    int found = 0;
    float withdrawAmount;

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    lseek(fp, 0, SEEK_END);

    while (read(file, &customer, sizeof(customer)) != 0)
    {
        if(customer.accountNumber == accountNumber)
        {
            break;
        }
    }
    int offset = lseek(file, -sizeof(struct Customer), SEEK_CUR);

    struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
    int lockStatus = fcntl(file, F_SETLKW, &fl);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter the amount to withdraw: ");
    writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));

    if(writeBytes == -1)
    {
        printf("Unable to write client\n");
    }
    else
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
        if(readBytes == -1)
        {
            printf("Unable to read from client\n");
        }
        else
        {
            if(lockStatus == -1)
            {
                printf("Unable to lock file\n");
                exit(0);
            }

            withdrawAmount = atof(readBuffer);

            lseek(file, 0, SEEK_SET);
            while(read(file, &customer, sizeof(customer)) != 0) {
                if (customer.accountNumber == accountNumber) {
                    break;
                }
            }
            lseek(file, -sizeof(struct Customer), SEEK_CUR);

            printf("requested to withdraw from %d account number: %.2f\n", accountNumber, withdrawAmount);

            if (customer.balance < withdrawAmount){
                bzero(writeBuffer, sizeof(writeBuffer));
                bzero(readBuffer, sizeof(readBuffer));
                
                printf("Insufficient balance in account: %d\n", accountNumber);

                sprintf(writeBuffer, "Insufficient funds!^");
                writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
                if(writeBytes == -1)
                {
                    printf("Unable to write to client\n");
                }

                fl.l_type = F_UNLCK;
                fcntl(file, F_SETLK, &fl);

                close(fp);
                close(file);
                return;
            }
            customer.balance -= withdrawAmount;

            bzero(transactionBuffer, sizeof(transactionBuffer));
            sprintf(transactionBuffer, "%.2f withdraw at %02d:%02d:%02d %d-%d-%d\n", withdrawAmount, current_time->tm_hour, current_time->tm_min,current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);

            bzero(th.hist, sizeof(th.hist));
            strcpy(th.hist, transactionBuffer);
            th.acc_no = customer.accountNumber;
            write(fp, &th, sizeof(th));
            write(file, &customer, sizeof(customer));

            fl.l_type = F_UNLCK;
            fcntl(file, F_SETLK, &fl);

            close(fp);
            close(file);

            bzero(readBuffer, sizeof(readBuffer));
            bzero(writeBuffer, sizeof(writeBuffer));
            printf("New balance of %d account number: %.2f\n", accountNumber, customer.balance);
            sprintf(writeBuffer, "Withdrawal successful!^");

            writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
            readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
            if(writeBytes == -1)
            {
                printf("Unable to write to client\n");
            }
        }
    }
    return;
}

// ======================= Apply loan =======================
void applyLoan(int connectionFD, int accountNumber)
{
    struct LoanDetails ld;
    char readBuffer[4096], writeBuffer[4096];
    int file1 = open(LOANPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (file1 == -1)
    {
        perror("Error opening loan file");
        return;
    }

    // ===== STEP 1: Generate Loan ID using global counter =====
    int loanID = getNextID(LOAN);
    if (loanID == -1)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Unable to generate Loan ID!^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        close(file1);
        return;
    }

    printf("Generated Loan ID: %d for account number: %d\n", loanID, accountNumber);

    // ===== STEP 2: Ask user for loan amount =====
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Loan Amount: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    int amount = atoi(readBuffer);

    // ===== STEP 3: Fill loan details =====
    ld.loanID = loanID;
    ld.accountNumber = accountNumber;
    ld.loanAmount = amount;
    ld.empID = -1;      // Not assigned yet
    ld.status = 0;      // 0 = Requested

    printf("Loan Application -> ID: %d, Acc: %d, Amount: %d\n",
           ld.loanID, ld.accountNumber, ld.loanAmount);

    // ===== STEP 4: Write to loan file =====
    ssize_t response = write(file1, &ld, sizeof(ld));
    close(file1);

    // ===== STEP 5: Acknowledge client =====
    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));

    if (response <= 0)
    {
        strcpy(writeBuffer, "Unable to apply for loan!^");
    }
    else
    {
        strcpy(writeBuffer, "Loan applied successfully!^");
    }

    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer)); // wait for client ACK
}

// ======================= Money Transfer =======================
void transferFunds(int connectionFD, int sourceAccount, int destAccount, float amount) {
    char readBuffer[4096], writeBuffer[4096], transactionBuffer[1024];

    struct Customer cs;
    struct trans_histroy th;
    int isTransfer = 0;

    time_t s, val = 1;
	struct tm* current_time;
	s = time(NULL);
	current_time = localtime(&s);

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    lseek(fp, 0, SEEK_END);
  
    int sourceFound = 0, destFound = 0;
    int srcOffset = -1, dstOffset = -1;

    while (read(file, &cs, sizeof(cs)) != 0)
    {
        if(cs.accountNumber == sourceAccount)
        {
            srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            read(file, &cs, sizeof(cs));
            sourceFound = 1;
        }
        if(cs.accountNumber == destAccount)
        {
            dstOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            read(file, &cs, sizeof(cs));
            destFound = 1;
        }

        if(sourceFound && destFound)
            break;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    lseek(file, srcOffset, SEEK_SET);
    read(file, &cs, sizeof(cs));

    if (cs.balance < amount) {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        printf("Insufficient funds\n");
        strcpy(writeBuffer, "Insufficient funds in the source account.^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));

        fl1.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl1);
        close(file);
        return;
    }

    printf("Current balance: %.2f\n", cs.balance);
    cs.balance -= amount; // Deduct from source account
    float srcBalance = cs.balance;

    bzero(transactionBuffer, sizeof(transactionBuffer));
    printf("%.2f transferred into acc no %d from acc no %d\n", amount, destAccount, sourceAccount);
    sprintf(transactionBuffer,"%.2f transferred into acc no %d at %02d:%02d:%02d %d-%d-%d", amount, destAccount, current_time->tm_hour,current_time->tm_min,current_time->tm_sec,(current_time->tm_year)+1900,(current_time->tm_mon)+1,current_time->tm_mday);
    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = sourceAccount;
    write(fp, &th, sizeof(th));

    lseek(file, -sizeof(struct Customer), SEEK_CUR);
    write(file, &cs, sizeof(cs));

    // Locking Destination Account
    struct flock fl2 = {F_WRLCK, SEEK_SET, dstOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl2);

    lseek(file, dstOffset, SEEK_SET);
    read(file, &cs, sizeof(cs));
    
    cs.balance += amount;

    lseek(file, -sizeof(struct Customer), SEEK_CUR);
    write(file, &cs, sizeof(cs));

    fl1.l_type = F_UNLCK;
    fl1.l_whence = SEEK_SET;
    fl1.l_start = srcOffset;
    fl1.l_len = sizeof(struct Customer);
    fl1.l_pid = getpid();

    fcntl(file, F_UNLCK, &fl1);

    fl2.l_type = F_UNLCK;
    fl2.l_whence = SEEK_SET;
    fl2.l_start = dstOffset;
    fl2.l_len = sizeof(struct Customer);
    fl2.l_pid = getpid();

    fcntl(file, F_UNLCK, &fl2);

    bzero(transactionBuffer, sizeof(transactionBuffer));
    sprintf(transactionBuffer,"%.2f credited by acc no %d at %02d:%02d:%02d %d-%d-%d", amount, sourceAccount, current_time->tm_hour,current_time->tm_min,current_time->tm_sec,(current_time->tm_year)+1900,(current_time->tm_mon)+1,current_time->tm_mday);
    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = destAccount;
    write(fp, &th, sizeof(th));

    close(file);

    bzero(writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    printf("Current balance of acc no %d: %.2f\n", sourceAccount, srcBalance);
    sprintf(writeBuffer, "Current Balance of %d is: %.2f^", sourceAccount, srcBalance);
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    return;
}

// ======================= View Transaction History =======================
void transactionHistory(int connectionFD, int accountNumber){
    char tempBuffer[4096];
    struct trans_histroy th;
    int maxTrans = 0;

    int file = open(HISTORYPATH, O_RDONLY | O_CREAT, 0644);

    bzero(writeBuffer, sizeof(writeBuffer));
    while(read(file, &th, sizeof(th)) != 0)
    {
        if(th.acc_no == accountNumber && maxTrans < 10)
        {
            maxTrans++;
            bzero(tempBuffer, sizeof(tempBuffer));
            strcpy(tempBuffer, th.hist);
            strcat(writeBuffer, tempBuffer);
        }
    }
    close(file);

    bzero(readBuffer, sizeof(readBuffer));
    strcat(writeBuffer, "^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ======================= Add Feedback =======================
void addFeedback(int connectionFD, int accountNumber)
{
    struct FeedBack fb;
    struct Customer customer;
    int found = 0;

    // ---------- Step 1: Find Customer Name using Account Number ----------
    int custFile = open(CUSPATH, O_RDONLY);
    if (custFile == -1)
    {
        perror("Error opening customer file");
        return;
    }

    while (read(custFile, &customer, sizeof(customer)) > 0)
    {
        if (customer.accountNumber == accountNumber)
        {
            // Copy the customer's name into feedback structure
            strcpy(fb.firstName, customer.firstName);
            strcpy(fb.lastName, customer.lastName);
            found = 1;
            break;
        }
    }
    close(custFile);

    if (!found)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Invalid account number. Unable to record feedback.^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        return;
    }

    // ---------- Step 2: Ask for Feedback ----------
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Please write your feedback: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));

    // Store feedback text safely
    strncpy(fb.feedback, readBuffer, sizeof(fb.feedback) - 1);
    fb.feedback[sizeof(fb.feedback) - 1] = '\0';  // Safety null terminator

    // ---------- Step 3: Save Feedback to File ----------
    int feedFile = open(FEEDPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (feedFile == -1)
    {
        perror("Error opening feedback file");
        return;
    }

    write(feedFile, &fb, sizeof(fb));
    close(feedFile);

    printf("Customer %s %s (Acc No: %d) added feedback.\n",
           fb.firstName, fb.lastName, accountNumber);

    // ---------- Step 4: Confirm to Client ----------
    bzero(writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    strcpy(writeBuffer, "Thank you for your feedback!^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ======================= Change Password =======================
int changePassword(int connectionFD, int accountNumber){
    char readBuffer[4096], writeBuffer[4096];

    char newPassword[20];

    struct Customer c;
    int file = open(CUSPATH,  O_CREAT | O_RDWR, 0644);
    
    lseek(file, 0, SEEK_SET);

    int srcOffset = -1, sourceFound = 0;

    while (read(file, &c, sizeof(c)) != 0)
    {
        if(c.accountNumber == accountNumber)
        {
            srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            sourceFound = 1;
        }
        if(sourceFound)
            break;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(newPassword, readBuffer);

    strcpy(c.password, newPassword);
    write(file, &c, sizeof(c));

    fl1.l_type = F_UNLCK;
    fl1.l_whence = SEEK_SET;
    fl1.l_start = srcOffset;
    fl1.l_len = sizeof(struct Customer);
    fl1.l_pid = getpid();

    fcntl(file, F_UNLCK, &fl1);
    close(file);

    printf("Customer %d changed password\n", accountNumber);
    return 1;
}

// ======================= Logout =======================
void logout(int connectionFD, int id){
    snprintf(semName, 50, "/sem_%d", id);

    sem_t *sema = sem_open(semName, 0);
    if (sema != SEM_FAILED) {
        sem_post(sema);
        sem_close(sema); 
        sem_unlink(semName);    
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}