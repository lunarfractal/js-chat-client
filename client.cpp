#define ASIO_STANDALONE

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client_no_tls.hpp>

#include <nlohmann/json.hpp>

const std::string COLOR_BLUE = "\033[34m";
const std::string COLOR_WHITE = "\033[37m";
const std::string COLOR_GREY = "\033[90m";
const std::string COLOR_RESET = "\033[0m";

typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::connection_hdl connection_hdl;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

void print_message(std::string username, std::string text, std::string timestamp) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col;

    std::string user_text = COLOR_BLUE + username + std::string(": ") + COLOR_WHITE + text + COLOR_RESET;
    //std::string time_text = COLOR_GREY + std::string("[") + timestamp + std::string("]") + COLOR_RESET;

    //int spacing = term_width - (int)(user_text.length() + time_text.length());
    //if (spacing < 1) spacing = 1;

    //std::string spaces(spacing, ' ');

    std::cout << user_text << std::endl;
}

void send_text_message(client* c, connection_hdl* connection, std::string msg) {
  c->send(*connection, msg, websocketpp::frame::opcode::text);
}

void on_message(client* c, connection_hdl hdl, message_ptr msg) {
  nlohmann::json data = nlohmann::json::parse(msg->get_payload());
  std::string type = data["type"];
  if(type == "chat")  {
    print_message(data["username"], data["text"], data["timestamp"]);
  }
  else if(type == "system") {
    print_message("<system>", data["text"], data.value("timestamp", ""));
  }
  else if(type == "history") {
    for(const auto& message: data["messages"]) {
      std::string _type = message["type"];
      if(_type == "chat")
      {
        print_message(message["username"], message["text"], message["timestamp"]);
      }
      else if(_type == "system")
      {
        print_message("<system>", message["text"], message.value("timestamp", ""));
      }
      else {
        std::cerr << "Unknown data message type " << type << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "Unknown server message type " << type << std::endl;
  }
}

void on_open(client* c, connection_hdl* connection, connection_hdl hdl) {
  print_message("<system>", "Connected!", "");
  *connection = hdl;
}

int main(int argc, char* argv[]) {
  std::string url = "ws://147.185.221.28:61429";

  if(argc == 2) {
    url = argv[1];
  }

  bool done = false;
  std::string input;

  client c;
  connection_hdl hdl;

  c.clear_access_channels(websocketpp::log::alevel::all);
  c.clear_error_channels(websocketpp::log::elevel::all);

  c.init_asio();

  //c.set_tls_init_handler(websocketpp::lib::bind(&on_tls_init));

  c.set_open_handler(websocketpp::lib::bind(&on_open, &c, &hdl, ::_1));

  c.set_message_handler(websocketpp::lib::bind(&on_message, &c, ::_1, ::_2));

  websocketpp::lib::error_code ec;
  auto connection = c.get_connection(url, ec);
  c.connect(connection);

  websocketpp::lib::thread t1(&client::run, &c);

  while (!done) {
    std::getline(std::cin, input);
    if (input == "/leave") {
      done = true;
      c.close(hdl, websocketpp::close::status::normal, "done");
    } else {
      send_text_message(&c, &hdl, "{\"type\":\"message\",\"content\":\"" + input + "\"}");
    }
  }

  t1.join();
}
