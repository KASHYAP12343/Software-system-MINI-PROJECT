#define ADMINNAME "Admin"
#define PASSWORD "PASS@123"
enum RecordType { RECORD_CUSTOMER, RECORD_EMPLOYEE };

int addEmployee(int connectionFD);
void modifyCE(int connectionFD, int modifyChoice);
void updateName(int connectionFD, int file, off_t recordOffset, int recordType);
void manageRole(int connectionFD);
void handleInterrupt(int signum);

volatile sig_atomic_t interrupted = 0;
char readBuffer[4096], writeBuffer[4096];

void adminMenu(int connectionFD)
{
    char password[20];
label1:
    bzero(writeBuffer, sizeof(writeBuffer));
    strcat(writeBuffer, "Enter password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(password, readBuffer);

    if(strcmp(PASSWORD, password) == 0)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(writeBuffer, "\nLogin Successfully^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(writeBuffer, "\nInvalid credential^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        goto label1;
    }

    while(1)
    {
        int modifyChoice;
        int choice;

        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ADMINMENU);
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        bzero(readBuffer, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        choice = atoi(readBuffer);
        printf("Admin entered: %d\n", choice);

        switch (choice) {
            case 1:
                // Add new bank employee
                if(!addEmployee(connectionFD))
                {
                    bzero(writeBuffer, sizeof(writeBuffer));
                    bzero(readBuffer, sizeof(readBuffer));
                    strcpy(writeBuffer, "Failed To Add Employee.^");
                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                    read(connectionFD, readBuffer, sizeof(readBuffer));
                }
                break;
            case 2:
                // Modify Customer/Employee details
                bzero(writeBuffer, sizeof(writeBuffer));
                strcpy(writeBuffer, "Enter 1 to Modify Customer\nEnter 2 to Modify Employee: ");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                
                bzero(readBuffer, sizeof(readBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
                modifyChoice = atoi(readBuffer);

                modifyCE(connectionFD, modifyChoice);
                break;
            case 3:
                // Manage User Roles
                manageRole(connectionFD);
                break;
            case 4:
                // Logout
                return;
            default:
                bzero(writeBuffer, sizeof(writeBuffer));
                bzero(readBuffer, sizeof(readBuffer));
                strcpy(writeBuffer, "Invalid choice! Please try again.^");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
        }
    }    
}

// ================ Add New Bank Employee ================



int addEmployee(int connectionFD)
{
    struct Employee emp;
    #define EMPLOYEE 1

    signal(SIGINT, handleInterrupt);
    signal(SIGTERM, handleInterrupt);

    interrupted = 0; // reset flag

    // ================== First Name ==================
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter First Name: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    if (read(connectionFD, readBuffer, sizeof(readBuffer)) <= 0 || interrupted) {
        printf("Operation cancelled during First Name entry.\n");
        return 0;
    }
    strcpy(emp.firstName, readBuffer);

    // ================== Last Name ==================
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Last Name: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    if (read(connectionFD, readBuffer, sizeof(readBuffer)) <= 0 || interrupted) {
        printf("Operation cancelled during Last Name entry.\n");
        return 0;
    }
    strcpy(emp.lastName, readBuffer);

    // ================== Password ==================
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    if (read(connectionFD, readBuffer, sizeof(readBuffer)) <= 0 || interrupted) {
        printf("Operation cancelled during Password entry.\n");
        return 0;
    }
    // strcpy(emp.password, crypt(readBuffer, HASHKEY));
    strcpy(emp.password, readBuffer);

    //  Only now generate Employee ID safely
    emp.empID = getNextID(EMPLOYEE);
    emp.role = 1;

    // ================== Write to File ==================
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if (file == -1) {
        perror("Error opening file");
        return 0;
    }

    lseek(file, 0, SEEK_END);
    int returnValue = write(file, &emp, sizeof(emp));
    close(file);

    if (returnValue == -1) {
        perror("Error writing employee");
        return 0;
    }

    printf(" Employee %s %s added successfully (ID: %d)\n",
           emp.firstName, emp.lastName, emp.empID);

    // Confirmation to client
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "Employee successfully added (ID: %d)^", emp.empID);
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));

    return 1;
}

void modifyCE(int connectionFD, int modifyChoice)
{
    if (modifyChoice == 1) // Modify Customer
    {
        struct Customer cus;
        int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
        if (file == -1)
        {
            printf("Error opening customer file!\n");
            return;
        }

        // Ask for Account Number
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Enter Account Number: ");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        bzero(readBuffer, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        int accNo = atoi(readBuffer);

        off_t srcOffset = -1;
        lseek(file, 0, SEEK_SET);
        while (read(file, &cus, sizeof(cus)) > 0)
        {
            if (cus.accountNumber == accNo)
            {
                srcOffset = lseek(file, -sizeof(cus), SEEK_CUR);
                break;
            }
        }

        if (srcOffset == -1)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid Account Number\n^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            close(file);
            return;
        }

        struct flock fl = {F_WRLCK, SEEK_SET, srcOffset, sizeof(cus), getpid()};
        fcntl(file, F_SETLKW, &fl);

        updateName(connectionFD, file, srcOffset, RECORD_CUSTOMER);

        fl.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl);
        close(file);
    }
    else if (modifyChoice == 2) // Modify Employee
    {
        struct Employee emp;
        int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
        if (file == -1)
        {
            printf("Error opening employee file!\n");
            return;
        }

        // Ask for Employee ID
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Enter Employee ID: ");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        bzero(readBuffer, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        int id = atoi(readBuffer);

        off_t srcOffset = -1;
        lseek(file, 0, SEEK_SET);
        while (read(file, &emp, sizeof(emp)) > 0)
        {
            if (emp.empID == id)
            {
                srcOffset = lseek(file, -sizeof(emp), SEEK_CUR);
                break;
            }
        }

        if (srcOffset == -1)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid Employee ID^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            close(file);
            return;
        }

        struct flock fl = {F_WRLCK, SEEK_SET, srcOffset, sizeof(emp), getpid()};
        fcntl(file, F_SETLKW, &fl);

        updateName(connectionFD, file, srcOffset, RECORD_EMPLOYEE);

        fl.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl);
        close(file);
    }
}
// Helper function to modify first/last/both names of a record
void updateName(int connectionFD, int file, off_t recordOffset, int recordType)
{
    int nameChoice;
    char newFirstName[20], newLastName[20];

    // Buffers for generic structures
    struct Employee emp;
    struct Customer cus;

    // Read the correct record from file
    lseek(file, recordOffset, SEEK_SET);
    if (recordType == RECORD_EMPLOYEE)
        read(file, &emp, sizeof(emp));
    else
        read(file, &cus, sizeof(cus));

    // Ask admin which name to change
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer,
           "Enter 1 to change First Name\n"
           "Enter 2 to change Last Name\n"
           "Enter 3 to change Both: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    nameChoice = atoi(readBuffer);

    // Update fields according to record type
    if (recordType == RECORD_EMPLOYEE)
    {
        if (nameChoice == 1 || nameChoice == 3)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter New First Name: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strcpy(newFirstName, readBuffer);

            strcpy(emp.firstName, newFirstName);
            printf("Admin updated FIRST name of employee (ID: %d) → %s\n",
                   emp.empID, newFirstName);
        }

        if (nameChoice == 2 || nameChoice == 3)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter New Last Name: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strcpy(newLastName, readBuffer);

            strcpy(emp.lastName, newLastName);
            printf("Admin updated LAST name of employee (ID: %d) → %s\n",
                   emp.empID, newLastName);
        }

        // Write back updated employee
        lseek(file, recordOffset, SEEK_SET);
        write(file, &emp, sizeof(emp));
    }
    else if (recordType == RECORD_CUSTOMER)
    {
        if (nameChoice == 1 || nameChoice == 3)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter New First Name: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strcpy(newFirstName, readBuffer);

            strcpy(cus.firstName, newFirstName);
            printf("Admin updated FIRST name of customer (AccNo: %d) → %s\n",
                   cus.accountNumber, newFirstName);
        }

        if (nameChoice == 2 || nameChoice == 3)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter New Last Name: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strcpy(newLastName, readBuffer);

            strcpy(cus.lastName, newLastName);
            printf("Admin updated LAST name of customer (AccNo: %d) → %s\n",
                   cus.accountNumber, newLastName);
        }

        // Write back updated customer
        lseek(file, recordOffset, SEEK_SET);
        write(file, &cus, sizeof(cus));
    }

    // Confirmation to client
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Record successfully updated.^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ===================== Manager Role ==================
void manageRole(int connectionFD)
{
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if(file == -1)
    {
        printf("Error opening file!\n");
        return ;
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    int id;
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    id = atoi(readBuffer);

 
    struct Employee emp;
    while(read(file, &emp, sizeof(emp)) != 0)
    {
        if(emp.empID == id)
        {
            int choice;
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter 1 to make manager\nEnter 2 to make employee: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            choice = atoi(readBuffer);

            lseek(file,-sizeof(struct Employee), SEEK_CUR);
            
            if(choice == 1)
            {
                printf("Admin made ID no: %d to  manager\n", id);
                emp.role = 0;            
                write(file, &emp, sizeof(emp));
            }
            else if(choice == 2)
            {
                printf("Admin made ID no: %d to employee\n", id);
                emp.role = 1;
                write(file, &emp, sizeof(emp));
            }
            close(file);
            
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            return ;
        }
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Invalid ID^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    return ;
}
void handleInterrupt(int signum) {
    interrupted = 1;
    printf("\n⚠️ Operation cancelled.\n");
}