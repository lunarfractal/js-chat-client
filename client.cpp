#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

#include <nlohmann/json.hpp>

const std::string COLOR_RESET     = "\033[0m";
const std::string COLOR_BLUE  = "\033[1;34m";
const std::string COLOR_WHITE      = "\033[0;37m";
const std::string COLOR_GREY = "\033[90m";

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::connection_hdl connection_hdl;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

tyepdef websocketpp::lib::asio::ssl::context ssl_context;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

void print_message(std::string &username, std::string &text, std::string &timestamp) {
  std::string half = COLOR_BLUE + username + ": " + COLOR_WHITE + text + COLOR_GREY + "   [" + timestamp + "]" + COLOR_RESET;
  std::cout << half << std::endl
}


void send_text_message(Client* client, ConnectionHdl* connection, std::string msg) {
  client->send(*connection, msg, websocketpp::frame::opcode::text);
}

void on_message(client* client, connection_hdl hdl, message_ptr msg) {
  nholmann::json data = nholmann::json::parse(msg->get_payload());
  std::string type = data["type"];
  switch(type) {
    case "chat":
    {
      print_message(data["username"], data["text"], data["timestamp"]);
      break;
    }
    case "system":
    {
      print_message("<system>", data["text"], data.value("timestamp", ""));
      break;
    }
    case "history":
    {
      for(const auto& message: data["messages"]) {
        switch(message["type"]) {
          case "chat":
          {
            print_message(data["username"], data["text"], data["timestamp"]);
            break:
          }
          case "system":
          {
            print_message("<system>", data["text"], data.value("timestamp", ""));
            break;
          }
          default:
          {
            std::cerr << "Unknown data message type " << type << std::endl;
            break;
          }
        }
      }
      break;
    }
    default:
    {
      std::cerr << "Unknown server message type " << type << std::endl;
      break;
    }
  }
}

void on_open(Client* client, connection_hdl* connection, connection_hdl hdl) {
  *connection = hdl;
}

websocketpp::lib::shared_ptr<ssl_context> on_tls_init() {
  auto ctx = websocketpp::lib::make_shared<ssl_context>(asio::ssl::context::sslv23);
  return ctx;
}

int main(int argc, char* argv[]) {
  std::string url = "ws://147.185.221.28:61429";

  if(argc == 2) {
    url = argv[1];
  }

  bool done = false;
  std::string input;
  std::string nick;
  client c;
  connection_hdl hdl;

  c.clear_access_channels(websocketpp::log::alevel::all);
  c.clear_error_channels(websocketpp::log::elevel::all);

  c.init_asio();

  c.set_tls_init_handler(websocketpp::lib::bind(&on_tls_init));

  c.set_open_handler(websocketpp::lib::bind(&on_open, &c, &hdl, ::_1));

  c.set_message_handler(websocketpp::lib::bind(&on_message, &client, ::_1, ::_2));

  websocketpp::lib::error_code ec;
  auto connection = c.get_connection(url, ec);
  c.connect(connection);

  websocketpp::lib::thread t1(&c::run, &c);

  std::cout << "Nick: ";
  std::getline(std::cin, nick);

  while (!done) {
    std::getline(std::cin, input);
    if (input == "/leave") {
      done = true;
      c.close(hdl, websocketpp::close::status::normal, "done");
    } else {

    }
  }

  t1.join();
}
