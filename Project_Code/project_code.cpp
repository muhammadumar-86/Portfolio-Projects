
#define NOMINMAX
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>

extern "C" {
#include <mysql.h>
}

#pragma warning(disable: 4996)
//  DB CREDENTIALS  - change to your settings
// --------------------------------------------------
const char* DB_HOST = "localhost";
const char* DB_USER = "root";
const char* DB_PASS = "root1234";
const char* DB_NAME = "CarRentalDB";

//  GLOBAL connection handle
// --------------------------------------------------
MYSQL* con = nullptr;


void clearScreen() {
    system("cls");
}

void pauseScreen() {
    std::cout << "\nPress ENTER to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void execSQL(const std::string& sql) {
    if (mysql_query(con, sql.c_str())) {
        std::cerr << "[SQL ERROR] " << mysql_error(con) << "\n";
    }
}

std::string escStr(const std::string& raw) {
    std::string buf(raw.size() * 2 + 1, '\0');
    unsigned long len = mysql_real_escape_string(con, &buf[0],
        raw.c_str(), (unsigned long)raw.size());
    buf.resize(len);
    return buf;
}

void hr() {
    std::cout << "============================================================\n";
}

//  DATABASE SETUP
// ==================================================

void setupDatabase() {

    execSQL(
        "CREATE TABLE IF NOT EXISTS Branches ("
        "branch_id   INT AUTO_INCREMENT PRIMARY KEY,"
        "branch_name VARCHAR(100) NOT NULL,"
        "city        VARCHAR(80)  NOT NULL,"
        "address     VARCHAR(200),"
        "phone       VARCHAR(20),"
        "created_at  TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"
    );

    execSQL(
        "CREATE TABLE IF NOT EXISTS Cars ("
        "car_id        INT AUTO_INCREMENT PRIMARY KEY,"
        "branch_id     INT NOT NULL,"
        "make          VARCHAR(50)  NOT NULL,"
        "model         VARCHAR(50)  NOT NULL,"
        "year          YEAR         NOT NULL,"
        "license_plate VARCHAR(20)  NOT NULL UNIQUE,"
        "category      ENUM('Economy','Compact','SUV','Luxury','Van') DEFAULT 'Economy',"
        "daily_rate    DECIMAL(8,2) NOT NULL,"
        "is_available  TINYINT(1)   DEFAULT 1,"
        "added_at      TIMESTAMP    DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (branch_id) REFERENCES Branches(branch_id))"
    );

    execSQL(
        "CREATE TABLE IF NOT EXISTS Users ("
        "user_id    INT AUTO_INCREMENT PRIMARY KEY,"
        "full_name  VARCHAR(100) NOT NULL,"
        "email      VARCHAR(100) NOT NULL UNIQUE,"
        "phone      VARCHAR(20),"
        "license_no VARCHAR(30)  NOT NULL UNIQUE,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"
    );

    execSQL(
        "CREATE TABLE IF NOT EXISTS Rentals ("
        "rental_id       INT AUTO_INCREMENT PRIMARY KEY,"
        "user_id         INT          NOT NULL,"
        "car_id          INT          NOT NULL,"
        "branch_id       INT          NOT NULL,"
        "rent_date       DATETIME     NOT NULL,"
        "expected_return DATETIME     NOT NULL,"
        "actual_return   DATETIME     DEFAULT NULL,"
        "daily_rate      DECIMAL(8,2) NOT NULL,"
        "base_charge     DECIMAL(10,2) DEFAULT NULL,"
        "fine_amount     DECIMAL(10,2) DEFAULT 0.00,"
        "total_charge    DECIMAL(10,2) DEFAULT NULL,"
        "status          ENUM('Active','Returned','Overdue') DEFAULT 'Active',"
        "FOREIGN KEY (user_id)   REFERENCES Users(user_id),"
        "FOREIGN KEY (car_id)    REFERENCES Cars(car_id),"
        "FOREIGN KEY (branch_id) REFERENCES Branches(branch_id))"
    );

    execSQL(
        "CREATE TABLE IF NOT EXISTS Payments ("
        "payment_id INT AUTO_INCREMENT PRIMARY KEY,"
        "rental_id  INT           NOT NULL,"
        "amount     DECIMAL(10,2) NOT NULL,"
        "paid_at    TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "method     ENUM('Cash','Card','Online') DEFAULT 'Cash',"
        "FOREIGN KEY (rental_id) REFERENCES Rentals(rental_id))"
    );

    // Seed branches if empty
    MYSQL_RES* res;
    mysql_query(con, "SELECT COUNT(*) FROM Branches");
    res = mysql_store_result(con);
    MYSQL_ROW row = mysql_fetch_row(res);
    int cnt = row ? atoi(row[0]) : 0;
    mysql_free_result(res);

    if (cnt == 0) {
        execSQL(
            "INSERT INTO Branches (branch_name, city, address, phone) VALUES "
            "('Downtown Branch','Karachi','Block 5, Clifton','021-1111111'),"
            "('Airport Branch','Lahore','Near Allama Iqbal Airport','042-2222222'),"
            "('Mall Branch','Islamabad','F-10 Markaz','051-3333333')"
        );
        std::cout << "[INFO] 3 demo branches inserted.\n";
    }

    // Seed cars if empty
    mysql_query(con, "SELECT COUNT(*) FROM Cars");
    res = mysql_store_result(con);
    row = mysql_fetch_row(res);
    cnt = row ? atoi(row[0]) : 0;
    mysql_free_result(res);

    if (cnt == 0) {
        execSQL(
            "INSERT INTO Cars (branch_id,make,model,year,license_plate,category,daily_rate) VALUES "
            "(1,'Toyota','Corolla',2022,'KHI-001','Compact',3500.00),"
            "(1,'Honda','Civic',2023,'KHI-002','Compact',4000.00),"
            "(1,'Suzuki','Alto',2021,'KHI-003','Economy',2000.00),"
            "(2,'Toyota','Fortuner',2023,'LHR-001','SUV',8000.00),"
            "(2,'Kia','Sportage',2022,'LHR-002','SUV',6500.00),"
            "(3,'Mercedes','C-Class',2023,'ISB-001','Luxury',15000.00),"
            "(3,'Toyota','Hiace',2022,'ISB-002','Van',7000.00)"
        );
        std::cout << "[INFO] 7 demo cars inserted.\n";
    }
}

// ==================================================
//  BRANCH MANAGEMENT
// ==================================================

void listBranches() {
    hr();
    std::cout << "  BRANCH LIST\n";
    hr();
    mysql_query(con, "SELECT branch_id, branch_name, city, phone FROM Branches ORDER BY branch_id");
    MYSQL_RES* res = mysql_store_result(con);
    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(25) << "Branch Name"
        << std::setw(20) << "City"
        << std::setw(15) << "Phone" << "\n";
    hr();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
        std::cout << std::setw(5) << row[0]
        << std::setw(25) << row[1]
        << std::setw(20) << row[2]
        << std::setw(15) << row[3] << "\n";
    mysql_free_result(res);
}

void addBranch() {
    hr();
    std::cout << "  ADD NEW BRANCH\n";
    hr();
    std::string name, city, addr, phone;
    std::cout << "Branch Name : "; std::getline(std::cin, name);
    std::cout << "City        : "; std::getline(std::cin, city);
    std::cout << "Address     : "; std::getline(std::cin, addr);
    std::cout << "Phone       : "; std::getline(std::cin, phone);

    std::string sql =
        "INSERT INTO Branches (branch_name,city,address,phone) VALUES ('"
        + escStr(name) + "','" + escStr(city) + "','"
        + escStr(addr) + "','" + escStr(phone) + "')";
    execSQL(sql);
    std::cout << "[OK] Branch added successfully.\n";
}

// ==================================================
//  CAR MANAGEMENT
// ==================================================

void listCars(bool onlyAvailable) {
    hr();
    std::cout << (onlyAvailable ? "  AVAILABLE CARS\n" : "  ALL CARS\n");
    hr();

    std::string sql =
        "SELECT c.car_id, b.branch_name, c.make, c.model, c.year, "
        "c.license_plate, c.category, c.daily_rate, "
        "(CASE WHEN c.is_available=1 THEN 'YES' ELSE 'RENTED' END) "
        "FROM Cars c JOIN Branches b ON c.branch_id=b.branch_id";
    if (onlyAvailable) sql += " WHERE c.is_available=1";
    sql += " ORDER BY c.car_id";

    mysql_query(con, sql.c_str());
    MYSQL_RES* res = mysql_store_result(con);

    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(20) << "Branch"
        << std::setw(12) << "Make"
        << std::setw(12) << "Model"
        << std::setw(6) << "Year"
        << std::setw(12) << "Plate"
        << std::setw(10) << "Category"
        << std::setw(12) << "Rate/Day"
        << std::setw(8) << "Avail" << "\n";
    hr();

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
        std::cout << std::setw(5) << row[0]
        << std::setw(20) << row[1]
        << std::setw(12) << row[2]
        << std::setw(12) << row[3]
        << std::setw(6) << row[4]
        << std::setw(12) << row[5]
        << std::setw(10) << row[6]
        << std::setw(12) << row[7]
        << std::setw(8) << row[8] << "\n";
    mysql_free_result(res);
}

void addCar() {
    hr();
    std::cout << "  ADD NEW CAR\n";
    hr();
    listBranches();

    int branchId;
    std::cout << "\nSelect Branch ID : "; std::cin >> branchId; std::cin.ignore();

    std::string make, model, plate, category;
    int year;
    double rate;

    std::cout << "Make             : "; std::getline(std::cin, make);
    std::cout << "Model            : "; std::getline(std::cin, model);
    std::cout << "Year             : "; std::cin >> year; std::cin.ignore();
    std::cout << "License Plate    : "; std::getline(std::cin, plate);
    std::cout << "Category (Economy/Compact/SUV/Luxury/Van) : ";
    std::getline(std::cin, category);
    std::cout << "Daily Rate (PKR) : "; std::cin >> rate; std::cin.ignore();

    std::ostringstream sql;
    sql << "INSERT INTO Cars (branch_id,make,model,year,license_plate,category,daily_rate) VALUES ("
        << branchId << ",'" << escStr(make) << "','" << escStr(model) << "',"
        << year << ",'" << escStr(plate) << "','" << escStr(category) << "'," << rate << ")";
    execSQL(sql.str());
    std::cout << "[OK] Car added successfully.\n";
}

// ==================================================
//  CUSTOMER MANAGEMENT
// ==================================================

void listUsers() {
    hr();
    std::cout << "  CUSTOMER LIST\n";
    hr();
    mysql_query(con,
        "SELECT user_id, full_name, email, phone, license_no FROM Users ORDER BY user_id");
    MYSQL_RES* res = mysql_store_result(con);

    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(25) << "Full Name"
        << std::setw(28) << "Email"
        << std::setw(15) << "Phone"
        << std::setw(15) << "License No" << "\n";
    hr();

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
        std::cout << std::setw(5) << row[0]
        << std::setw(25) << row[1]
        << std::setw(28) << row[2]
        << std::setw(15) << row[3]
        << std::setw(15) << row[4] << "\n";
    mysql_free_result(res);
}

void registerUser() {
    hr();
    std::cout << "  REGISTER NEW CUSTOMER\n";
    hr();
    std::string name, email, phone, license;
    std::cout << "Full Name          : "; std::getline(std::cin, name);
    std::cout << "Email              : "; std::getline(std::cin, email);
    std::cout << "Phone              : "; std::getline(std::cin, phone);
    std::cout << "Driving License No : "; std::getline(std::cin, license);

    std::string sql =
        "INSERT INTO Users (full_name,email,phone,license_no) VALUES ('"
        + escStr(name) + "','" + escStr(email) + "','"
        + escStr(phone) + "','" + escStr(license) + "')";
    execSQL(sql);
    std::cout << "[OK] Customer registered. ID = " << mysql_insert_id(con) << "\n";
}

// ==================================================
//  RENTAL MANAGEMENT
// ==================================================

void rentCar() {
    hr();
    std::cout << "  RENT A CAR\n";
    hr();

    listUsers();
    int userId;
    std::cout << "\nEnter Customer ID : "; std::cin >> userId; std::cin.ignore();

    std::string chkUser =
        "SELECT user_id FROM Users WHERE user_id=" + std::to_string(userId);
    mysql_query(con, chkUser.c_str());
    MYSQL_RES* r = mysql_store_result(con);
    if (mysql_num_rows(r) == 0) {
        mysql_free_result(r);
        std::cout << "[ERROR] Customer not found.\n";
        return;
    }
    mysql_free_result(r);

    listCars(true);
    int carId;
    std::cout << "\nEnter Car ID to rent : "; std::cin >> carId; std::cin.ignore();

    std::string chkCar =
        "SELECT car_id, daily_rate, branch_id, is_available FROM Cars WHERE car_id="
        + std::to_string(carId);
    mysql_query(con, chkCar.c_str());
    r = mysql_store_result(con);
    MYSQL_ROW row = mysql_fetch_row(r);
    if (!row) {
        mysql_free_result(r);
        std::cout << "[ERROR] Car not found.\n";
        return;
    }
    if (atoi(row[3]) == 0) {
        mysql_free_result(r);
        std::cout << "[ERROR] Car is already rented.\n";
        return;
    }

    double dailyRate = atof(row[1]);
    int branchId = atoi(row[2]);
    mysql_free_result(r);

    int days;
    std::cout << "Number of rental days : "; std::cin >> days; std::cin.ignore();
    if (days <= 0) {
        std::cout << "[ERROR] Invalid number of days.\n";
        return;
    }

    time_t rentTime = time(nullptr);
    time_t retTime = rentTime + (time_t)days * 86400;

    char rentBuf[20], retBuf[20];
    strftime(rentBuf, sizeof(rentBuf), "%Y-%m-%d %H:%M:%S", localtime(&rentTime));
    strftime(retBuf, sizeof(retBuf), "%Y-%m-%d %H:%M:%S", localtime(&retTime));

    std::ostringstream sql;
    sql << "INSERT INTO Rentals "
        "(user_id,car_id,branch_id,rent_date,expected_return,daily_rate) VALUES ("
        << userId << "," << carId << "," << branchId
        << ",'" << rentBuf << "','" << retBuf << "'," << dailyRate << ")";
    execSQL(sql.str());
    int rentalId = (int)mysql_insert_id(con);

    execSQL("UPDATE Cars SET is_available=0 WHERE car_id=" + std::to_string(carId));

    std::cout << "\n[OK] Rental created!\n";
    std::cout << "  Rental ID   : " << rentalId << "\n";
    std::cout << "  Rented on   : " << rentBuf << "\n";
    std::cout << "  Return by   : " << retBuf << "\n";
    std::cout << "  Rate/Day    : PKR " << std::fixed << std::setprecision(2) << dailyRate << "\n";
    std::cout << "  Est. Total  : PKR " << dailyRate * days << "\n";
}

void returnCar() {
    hr();
    std::cout << "  RETURN A CAR\n";
    hr();

    mysql_query(con,
        "SELECT r.rental_id, u.full_name, c.license_plate, c.make, c.model, "
        "r.rent_date, r.expected_return, r.daily_rate "
        "FROM Rentals r "
        "JOIN Users u ON r.user_id=u.user_id "
        "JOIN Cars  c ON r.car_id=c.car_id "
        "WHERE r.status='Active' OR r.status='Overdue' "
        "ORDER BY r.rental_id");

    MYSQL_RES* res = mysql_store_result(con);

    std::cout << std::left
        << std::setw(8) << "RentID"
        << std::setw(22) << "Customer"
        << std::setw(12) << "Plate"
        << std::setw(16) << "Car"
        << std::setw(22) << "Rented On"
        << std::setw(22) << "Expected Return"
        << "Rate\n";
    hr();

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::string carName = std::string(row[3]) + " " + row[4];
        std::cout << std::setw(8) << row[0]
            << std::setw(22) << row[1]
            << std::setw(12) << row[2]
            << std::setw(16) << carName
            << std::setw(22) << row[5]
            << std::setw(22) << row[6]
            << row[7] << "\n";
    }
    mysql_free_result(res);

    int rentalId;
    std::cout << "\nEnter Rental ID to return : "; std::cin >> rentalId; std::cin.ignore();

    std::string fetchSql =
        "SELECT r.car_id, r.daily_rate, r.expected_return, r.rent_date "
        "FROM Rentals r "
        "WHERE r.rental_id=" + std::to_string(rentalId) +
        " AND (r.status='Active' OR r.status='Overdue')";
    mysql_query(con, fetchSql.c_str());
    res = mysql_store_result(con);
    row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        std::cout << "[ERROR] Rental not found or already returned.\n";
        return;
    }

    int    carId = atoi(row[0]);
    double dailyRate = atof(row[1]);
    std::string expRet = row[2];
    std::string rentDt = row[3];
    mysql_free_result(res);

    // Parse expected_return
    struct tm tmExp = {};
    sscanf(expRet.c_str(), "%d-%d-%d %d:%d:%d",
        &tmExp.tm_year, &tmExp.tm_mon, &tmExp.tm_mday,
        &tmExp.tm_hour, &tmExp.tm_min, &tmExp.tm_sec);
    tmExp.tm_year -= 1900;
    tmExp.tm_mon -= 1;
    time_t expectedT = mktime(&tmExp);

    // Parse rent_date
    struct tm tmRent = {};
    sscanf(rentDt.c_str(), "%d-%d-%d %d:%d:%d",
        &tmRent.tm_year, &tmRent.tm_mon, &tmRent.tm_mday,
        &tmRent.tm_hour, &tmRent.tm_min, &tmRent.tm_sec);
    tmRent.tm_year -= 1900;
    tmRent.tm_mon -= 1;
    time_t rentedT = mktime(&tmRent);

    time_t nowT = time(nullptr);

    double totalDays = difftime(nowT, rentedT) / 86400.0;
    if (totalDays < 1.0) totalDays = 1.0;

    double baseCharge = dailyRate * totalDays;
    double fine = 0.0;
    int overdueDays = 0;

    if (nowT > expectedT) {
        overdueDays = (int)ceil(difftime(nowT, expectedT) / 86400.0);
        fine = overdueDays * dailyRate * 0.20;
    }

    double totalCharge = baseCharge + fine;

    char nowBuf[20];
    strftime(nowBuf, sizeof(nowBuf), "%Y-%m-%d %H:%M:%S", localtime(&nowT));

    std::cout << "\n--- RETURN SUMMARY ---\n";
    std::cout << "  Actual Return Date : " << nowBuf << "\n";
    std::cout << "  Total Days Used    : " << std::fixed << std::setprecision(1) << totalDays << "\n";
    std::cout << "  Base Charge        : PKR " << std::setprecision(2) << baseCharge << "\n";
    if (overdueDays > 0) {
        std::cout << "  Overdue Days       : " << overdueDays << "\n";
        std::cout << "  Fine (20%/day)     : PKR " << fine << "\n";
    }
    std::cout << "  ---------------------\n";
    std::cout << "  TOTAL DUE          : PKR " << totalCharge << "\n";

    // Update rental record
    std::ostringstream updSql;
    updSql << "UPDATE Rentals SET "
        << "actual_return='" << nowBuf << "',"
        << "base_charge=" << baseCharge << ","
        << "fine_amount=" << fine << ","
        << "total_charge=" << totalCharge << ","
        << "status='Returned' "
        << "WHERE rental_id=" << rentalId;
    execSQL(updSql.str());

    // Make car available again
    execSQL("UPDATE Cars SET is_available=1 WHERE car_id=" + std::to_string(carId));

    // Record payment
    std::string payMethod;
    std::cout << "\nPayment Method (Cash/Card/Online) : ";
    std::getline(std::cin, payMethod);
    if (payMethod != "Cash" && payMethod != "Card" && payMethod != "Online")
        payMethod = "Cash";

    std::ostringstream paySql;
    paySql << "INSERT INTO Payments (rental_id,amount,method) VALUES ("
        << rentalId << "," << totalCharge << ",'" << escStr(payMethod) << "')";
    execSQL(paySql.str());

    std::cout << "[OK] Car returned and payment recorded.\n";
}

// ==================================================
//  OVERDUE CHECK
// ==================================================

void checkOverdue() {
    execSQL(
        "UPDATE Rentals SET status='Overdue' "
        "WHERE status='Active' AND expected_return < NOW()"
    );

    hr();
    std::cout << "  OVERDUE RENTALS\n";
    hr();

    mysql_query(con,
        "SELECT r.rental_id, u.full_name, u.phone, c.license_plate, "
        "r.expected_return, "
        "TIMESTAMPDIFF(DAY, r.expected_return, NOW()) AS overdue_days, "
        "r.daily_rate * 0.20 * TIMESTAMPDIFF(DAY, r.expected_return, NOW()) AS est_fine "
        "FROM Rentals r "
        "JOIN Users u ON r.user_id=u.user_id "
        "JOIN Cars  c ON r.car_id=c.car_id "
        "WHERE r.status='Overdue' "
        "ORDER BY overdue_days DESC");

    MYSQL_RES* res = mysql_store_result(con);

    if (mysql_num_rows(res) == 0) {
        std::cout << "  No overdue rentals.\n";
        mysql_free_result(res);
        return;
    }

    std::cout << std::left
        << std::setw(8) << "RentID"
        << std::setw(22) << "Customer"
        << std::setw(14) << "Phone"
        << std::setw(12) << "Plate"
        << std::setw(22) << "Was Due"
        << std::setw(14) << "Overdue Days"
        << "Est. Fine (PKR)\n";
    hr();

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
        std::cout << std::setw(8) << row[0]
        << std::setw(22) << row[1]
        << std::setw(14) << row[2]
        << std::setw(12) << row[3]
        << std::setw(22) << row[4]
        << std::setw(14) << row[5]
        << (row[6] ? row[6] : "0.00") << "\n";
    mysql_free_result(res);
}

// ==================================================
//  REPORTS
// ==================================================

void rentalHistory() {
    hr();
    std::cout << "  RENTAL HISTORY (last 50)\n";
    hr();

    mysql_query(con,
        "SELECT r.rental_id, u.full_name, c.make, c.model, c.license_plate, "
        "r.rent_date, r.expected_return, r.actual_return, "
        "r.total_charge, r.fine_amount, r.status "
        "FROM Rentals r "
        "JOIN Users u ON r.user_id=u.user_id "
        "JOIN Cars  c ON r.car_id=c.car_id "
        "ORDER BY r.rental_id DESC LIMIT 50");

    MYSQL_RES* res = mysql_store_result(con);

    std::cout << std::left
        << std::setw(6) << "ID"
        << std::setw(20) << "Customer"
        << std::setw(20) << "Car"
        << std::setw(12) << "Plate"
        << std::setw(20) << "Rented"
        << std::setw(20) << "Expected"
        << std::setw(20) << "Returned"
        << std::setw(12) << "Total PKR"
        << std::setw(10) << "Fine PKR"
        << "Status\n";
    hr();

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::string car = std::string(row[2]) + " " + row[3];
        std::cout << std::setw(6) << row[0]
            << std::setw(20) << row[1]
            << std::setw(20) << car
            << std::setw(12) << row[4]
            << std::setw(20) << row[5]
            << std::setw(20) << row[6]
            << std::setw(20) << (row[7] ? row[7] : "-")
            << std::setw(12) << (row[8] ? row[8] : "-")
            << std::setw(10) << (row[9] ? row[9] : "0.00")
            << row[10] << "\n";
    }
    mysql_free_result(res);
}

void revenueReport() {
    hr();
    std::cout << "  REVENUE REPORT (by Branch)\n";
    hr();

    mysql_query(con,
        "SELECT b.branch_name, "
        "COUNT(r.rental_id) AS total_rentals, "
        "IFNULL(SUM(r.base_charge),0) AS base_rev, "
        "IFNULL(SUM(r.fine_amount),0) AS fine_rev, "
        "IFNULL(SUM(r.total_charge),0) AS total_rev "
        "FROM Branches b "
        "LEFT JOIN Rentals r ON b.branch_id=r.branch_id AND r.status='Returned' "
        "GROUP BY b.branch_id ORDER BY total_rev DESC");

    MYSQL_RES* res = mysql_store_result(con);

    std::cout << std::left
        << std::setw(25) << "Branch"
        << std::setw(15) << "Completed"
        << std::setw(18) << "Base Revenue"
        << std::setw(18) << "Fines Collected"
        << "Total Revenue\n";
    hr();

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
        std::cout << std::setw(25) << row[0]
        << std::setw(15) << row[1]
        << std::setw(18) << row[2]
        << std::setw(18) << row[3]
        << row[4] << "\n";
    mysql_free_result(res);
}

// ==================================================
//  MENUS
// ==================================================

void branchMenu() {
    int ch;
    do {
        clearScreen();
        hr();
        std::cout << "  BRANCH MANAGEMENT\n";
        hr();
        std::cout << "  1. List All Branches\n";
        std::cout << "  2. Add New Branch\n";
        std::cout << "  0. Back\n";
        hr();
        std::cout << "  Choice : "; std::cin >> ch; std::cin.ignore();
        clearScreen();
        switch (ch) {
        case 1: listBranches(); pauseScreen(); break;
        case 2: addBranch();    pauseScreen(); break;
        }
    } while (ch != 0);
}

void carMenu() {
    int ch;
    do {
        clearScreen();
        hr();
        std::cout << "  CAR MANAGEMENT\n";
        hr();
        std::cout << "  1. List All Cars\n";
        std::cout << "  2. List Available Cars\n";
        std::cout << "  3. Add New Car\n";
        std::cout << "  0. Back\n";
        hr();
        std::cout << "  Choice : "; std::cin >> ch; std::cin.ignore();
        clearScreen();
        switch (ch) {
        case 1: listCars(false); pauseScreen(); break;
        case 2: listCars(true);  pauseScreen(); break;
        case 3: addCar();        pauseScreen(); break;
        }
    } while (ch != 0);
}

void customerMenu() {
    int ch;
    do {
        clearScreen();
        hr();
        std::cout << "  CUSTOMER MANAGEMENT\n";
        hr();
        std::cout << "  1. List All Customers\n";
        std::cout << "  2. Register New Customer\n";
        std::cout << "  0. Back\n";
        hr();
        std::cout << "  Choice : "; std::cin >> ch; std::cin.ignore();
        clearScreen();
        switch (ch) {
        case 1: listUsers();    pauseScreen(); break;
        case 2: registerUser(); pauseScreen(); break;
        }
    } while (ch != 0);
}

void rentalMenu() {
    int ch;
    do {
        clearScreen();
        hr();
        std::cout << "  RENTAL OPERATIONS\n";
        hr();
        std::cout << "  1. Rent a Car\n";
        std::cout << "  2. Return a Car\n";
        std::cout << "  3. View Overdue Rentals\n";
        std::cout << "  4. Full Rental History\n";
        std::cout << "  0. Back\n";
        hr();
        std::cout << "  Choice : "; std::cin >> ch; std::cin.ignore();
        clearScreen();
        switch (ch) {
        case 1: rentCar();       pauseScreen(); break;
        case 2: returnCar();     pauseScreen(); break;
        case 3: checkOverdue();  pauseScreen(); break;
        case 4: rentalHistory(); pauseScreen(); break;
        }
    } while (ch != 0);
}

void reportMenu() {
    int ch;
    do {
        clearScreen();
        hr();
        std::cout << "  REPORTS\n";
        hr();
        std::cout << "  1. Revenue Report (by Branch)\n";
        std::cout << "  2. Rental History (last 50)\n";
        std::cout << "  3. Overdue Rentals\n";
        std::cout << "  0. Back\n";
        hr();
        std::cout << "  Choice : "; std::cin >> ch; std::cin.ignore();
        clearScreen();
        switch (ch) {
        case 1: revenueReport(); pauseScreen(); break;
        case 2: rentalHistory(); pauseScreen(); break;
        case 3: checkOverdue();  pauseScreen(); break;
        }
    } while (ch != 0);
}

void mainMenu() {
    int ch;
    do {
        clearScreen();
        hr();
        std::cout << "    CAR RENTAL MANAGEMENT SYSTEM\n";
        hr();
        std::cout << "  1. Branch Management\n";
        std::cout << "  2. Car Management\n";
        std::cout << "  3. Customer Management\n";
        std::cout << "  4. Rental Operations\n";
        std::cout << "  5. Reports\n";
        std::cout << "  0. Exit\n";
        hr();
        std::cout << "  Choice : "; std::cin >> ch; std::cin.ignore();

        switch (ch) {
        case 1: branchMenu();   break;
        case 2: carMenu();      break;
        case 3: customerMenu(); break;
        case 4: rentalMenu();   break;
        case 5: reportMenu();   break;
        case 0: std::cout << "\nGoodbye!\n"; break;
        default:
            std::cout << "Invalid choice.\n";
            pauseScreen();
            break;
        }
    } while (ch != 0);
}

int main() {
    con = mysql_init(NULL);
    if (!con) {
        std::cerr << "mysql_init() failed\n";
        return 1;
    }

    if (!mysql_real_connect(con, DB_HOST, DB_USER, DB_PASS,
        NULL, 0, NULL, 0)) {
        std::cerr << "Connection failed: " << mysql_error(con) << "\n";
        mysql_close(con);
        return 1;
    }
    std::cout << "[OK] Connected to MySQL.\n";

    std::string createDB = std::string("CREATE DATABASE IF NOT EXISTS ") + DB_NAME;
    execSQL(createDB);

    if (mysql_select_db(con, DB_NAME)) {
        std::cerr << "Cannot select DB: " << mysql_error(con) << "\n";
        mysql_close(con);
        return 1;
    }
    std::cout << "[OK] Using database '" << DB_NAME << "'.\n";

    setupDatabase();
    std::cout << "[OK] All tables ready.\n\n";

    mainMenu();

    mysql_close(con);
    return 0;
}