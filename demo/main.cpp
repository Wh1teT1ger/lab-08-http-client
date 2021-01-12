// Copyright 2020 Burylov Denis <burylov01@mail.ru>

#include <header.hpp>

io_service service;

class talk_to_svr {
  ip::tcp::socket socket_;
  int already_read_;
  char buff_[1024];
  std::string username_;

 public:
  talk_to_svr(const std::string& username)
      : socket_(service), username_(username) {}

  void connect(const ip::tcp::endpoint& endpoint) { socket_.connect(endpoint); }

  void loop() {
    write("login " + username_ + "\n");
    read_answer();
    std::random_device rd;
    std::mt19937 mersenne(rd());
    while (true) {
      std::string command;
      write_request();
      read_answer();
      int sleep_time = mersenne() % 7000;
      std::cout << username_ << " postpone ping " << sleep_time << " ms"
                << std::endl;
      boost::this_thread::sleep(boost::posix_time::millisec(sleep_time));
    }
  }
  std::string username() const { return username_; }

 private:
  void write_request() { write("ping\n"); }
  void read_answer() {
    already_read_ = 0;
    read(socket_, buffer(buff_),
         boost::bind(&talk_to_svr::read_complete, this, _1, _2));
    std::string msg(buff_, already_read_);
    if (msg.find("login ") == 0)
      login();
    else if (msg.find("ping") == 0)
      ping(msg);
    else if (msg.find("clients ") == 0)
      client_list(msg);
    else
      std::cerr << "invalid msg " << msg << std::endl;
  }

  void login() {
    std::cout << username_ << " logged in" << std::endl;
    request_client_list();
  }

  void ping(const std::string& msg) {
    std::istringstream in(msg);
    std::string answer;
    in >> answer >> answer;
    if (answer == "client_list_changed") request_client_list();
  }

  void client_list(const std::string& msg) {
    std::string clients = msg.substr(8);
    std::cout << username_ << ", client list:" << clients;
  }

  void request_client_list() {
    write("client_list\n");
    read_answer();
  }

  void write(const std::string& msg) { socket_.write_some(buffer(msg)); }

  size_t read_complete(const boost::system::error_code& err, size_t bytes) {
    if (err) return 0;
    already_read_ = bytes;
    bool found = std::find(buff_, buff_ + bytes, '\n') < buff_ + bytes;
    return found ? 0 : 1;
  }
};

void run_client(const std::string& client_name) {
  talk_to_svr client(client_name);
  try {
    client.connect(
        ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 8001));
    client.loop();
  } catch (boost::system::system_error& err) {
    std::cout << client.username()<<" disconected " << ": " << err.what()
              << std::endl;
  }
}

int main() {
  boost::thread_group threads;
  std::vector<std::string> names = {"Ivan", "Denis", "Petr"};
  for (const auto& name : names) {
    threads.create_thread(boost::bind(run_client, name));
    boost::this_thread::sleep(boost::posix_time::millisec(100));
  }
  threads.join_all();
}