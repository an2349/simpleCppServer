#include <condition_variable>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <vector>
#include <memory>
#include "../configs.h"

using namespace std;
using namespace sql;

class DBConnection {
private:
    shared_ptr<Connection> conn;

public:
    bool connect() {
        try {
            Driver* driver = get_driver_instance();
            conn = shared_ptr<Connection>(
                driver->connect(DB_HOST, DB_USER, DB_PASS)
            );
            conn->setSchema(DB_NAME);
            return true;
        } catch (SQLException& e) {
            return false;
        }
    }

    shared_ptr<Connection> get() { return conn; }
};


class DBPool {
private:
    vector<shared_ptr<DBConnection>> pool;
    mutex mtx;
    condition_variable cv;

public:
    DBPool() {
        for (int i = 0; i < MAX_CONN; ++i) {
            auto conn = make_shared<DBConnection>();
            if (conn->connect()) {  // không truyền param
                pool.push_back(conn);
            } else {
                cerr << "Khởi tạo DB connection thất bại\n";
            }
        }
    }

    shared_ptr<DBConnection> getConn() {
        unique_lock<mutex> lock(mtx);
        //cv.wait(lock, [this]() { return !pool.empty(); });
        if (!cv.wait_for(lock, chrono::seconds(TIME_OUT), [this]() { return !pool.empty(); })) {
            return nullptr;  // hoặc throw runtime_error("Timeout khi lấy connection");
        }
        auto conn = pool.back();
        pool.pop_back();
        return conn;
    }

    void closeConn(shared_ptr<DBConnection> conn) {
        unique_lock<mutex> lock(mtx);
        pool.push_back(conn);
        cv.notify_one();
    }
};
