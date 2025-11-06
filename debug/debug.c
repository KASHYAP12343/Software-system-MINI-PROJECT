#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../AllStructures/allstructure.h"

// File paths
#define EMPPATH "../Data/employees.txt"
#define CUSPATH "../Data/customers.txt"
#define LOANPATH "../Data/loanDetails.txt"
#define COUNTERPATH "../Data/Counter.txt"
#define HISTORYPATH "../Data/trans_hist.txt"
#define FEEDPATH "../Data/feedback.txt"

int main() {
    FILE *f1 = fopen(CUSPATH, "rb");
    FILE *f2 = fopen(EMPPATH, "rb");
    FILE *f3 = fopen(LOANPATH, "rb");
    FILE *f4 = fopen(COUNTERPATH, "rb");
    FILE *f5 = fopen(HISTORYPATH, "rb");
    FILE *f6 = fopen(FEEDPATH, "rb");

    struct Customer temp;
    struct Employee temp1;
    struct LoanDetails temp2;
    struct Counter ct;
    struct trans_histroy th;
    struct FeedBack fb;

    printf("\n========== DATABASE DEBUG TOOL ==========\n\n");

    // ---------------- CUSTOMER ----------------
    printf("========= Customers =========\n");
    if (!f1) {
        perror("Customer file not found");
    } else {
        int found = 0;
        while (fread(&temp, sizeof(temp), 1, f1) == 1) {
            found = 1;
            printf("First Name: %s\n", temp.firstName);
            printf("Last Name: %s\n", temp.lastName);
            printf("Account Number: %d\n", temp.accountNumber);
            printf("Balance: %.2f\n", temp.balance);
            printf("Status: %d\n", temp.activeStatus);
            printf("Password: %s\n", temp.password);
            printf("----------------------------------\n");
        }
        if (!found)
            printf("(No customers found)\n");
        fclose(f1);
    }

    // ---------------- EMPLOYEE ----------------
    printf("\n========= Employees =========\n");
    if (!f2) {
        perror("Employee file not found");
    } else {
        int found = 0;
        while (fread(&temp1, sizeof(temp1), 1, f2) == 1) {
            found = 1;
            printf("Employee ID: %d\n", temp1.empID);
            printf("First Name: %s\n", temp1.firstName);
            printf("Last Name: %s\n", temp1.lastName);
            printf("Role: %d\n", temp1.role);
            printf("Password: %s\n", temp1.password);
            printf("----------------------------------\n");
        }
        if (!found)
            printf("(No employees found)\n");
        fclose(f2);
    }

    // ---------------- LOAN DETAILS ----------------
    printf("\n========= Loan Details =========\n");
    if (!f3) {
        perror("Loan file not found");
    } else {
        int found = 0;
        while (fread(&temp2, sizeof(temp2), 1, f3) == 1) {
            found = 1;
            printf("Loan ID: %d\n", temp2.loanID);
            printf("Account Number: %d\n", temp2.accountNumber);
            printf("Employee ID: %d\n", temp2.empID);
            printf("Loan Amount: %d\n", temp2.loanAmount);
            printf("Status: %d\n", temp2.status);
            printf("----------------------------------\n");
        }
        if (!found)
            printf("(No loan records found)\n");
        fclose(f3);
    }

    // ---------------- COUNTER ----------------
    printf("\n========= Counters =========\n");
    if (!f4) {
        perror("Counter file not found");
    } else {
        char buffer[256];
        size_t bytes = fread(buffer, 1, sizeof(buffer) - 1, f4);
        buffer[bytes] = '\0';
        if (bytes > 0)
            printf("%s\n", buffer);
        else
            printf("(No counter data found)\n");
        fclose(f4);
    }

    // ---------------- TRANSACTION HISTORY ----------------
    printf("\n========= Transaction History =========\n");
    if (!f5) {
        perror("Transaction history file not found");
    } else {
        int found = 0;
        while (fread(&th, sizeof(th), 1, f5) == 1) {
            found = 1;
            printf("Account Number: %d\n", th.acc_no);
            printf("History:\n%s\n", th.hist);
            printf("----------------------------------\n");
        }
        if (!found)
            printf("(No transaction history found)\n");
        fclose(f5);
    }

    // ---------------- FEEDBACK ----------------
    printf("\n========= Feedback =========\n");
    if (!f6) {
        perror("Feedback file not found");
    } else {
        int found = 0;
        while (fread(&fb, sizeof(fb), 1, f6) == 1) {
            found = 1;
            printf("Customer: %s %s\n", fb.firstName, fb.lastName);
            printf("Feedback: %s\n", fb.feedback);
            printf("----------------------------------\n");
        }
        if (!found)
            printf("(No feedback found)\n");
        fclose(f6);
    }

    printf("\n========== END OF DATABASE ==========\n");
    return 0;
}
