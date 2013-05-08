#ifndef example_depth_feed_connection_h
#define example_depth_feed_connection_h

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <Application/QuickFAST.h>
#include <Common/WorkingBuffer.h>
#include <deque>
#include <set>
#include <Codecs/Encoder.h>
#include <Codecs/TemplateRegistry_fwd.h>

namespace liquibook { namespace examples {
  typedef boost::shared_ptr<QuickFAST::WorkingBuffer> WorkingBufferPtr;
  typedef std::deque<WorkingBufferPtr> WorkingBuffers;
  typedef boost::array<unsigned char, 128> Buffer;
  typedef boost::shared_ptr<Buffer> BufferPtr;
  typedef boost::function<bool (BufferPtr, size_t)> MessageHandler;
  typedef boost::function<void ()> ResetHandler;
  typedef boost::function<void (const boost::system::error_code& error,
                                std::size_t bytes_transferred)> SendHandler;
  typedef boost::function<void (const boost::system::error_code& error,
                                std::size_t bytes_transferred)> RecvHandler;

  class DepthFeedConnection;

  // Socket connection
  class DepthFeedSession : boost::noncopyable {
  public:
    DepthFeedSession(boost::asio::io_service& ios,
                     DepthFeedConnection* connection,
                     QuickFAST::Codecs::TemplateRegistryPtr& templates);

    ~DepthFeedSession();

    bool connected() const { return connected_; }
    void set_connected() { connected_ = true; }

    boost::asio::ip::tcp::socket& socket() { return socket_; }

    // Send a trade messsage to all clients
    void send_trade(QuickFAST::Messages::FieldSet& message);

    // Send an incremental update - if this client has handled this symbol
    //   return true if handled
    bool send_incr_update(const std::string& symbol,
                          QuickFAST::Messages::FieldSet& message);

    // Send a full update - if the client has not yet received for this symbol
    void send_full_update(const std::string& symbol,
                          QuickFAST::Messages::FieldSet& message);
    void on_accept(const boost::system::error_code& error);
  private:       
    bool connected_;
    uint64_t seq_num_;

    boost::asio::io_service& ios_;
    boost::asio::ip::tcp::socket socket_;
    DepthFeedConnection* connection_;
    QuickFAST::Codecs::Encoder encoder_;

    typedef std::set<std::string> StringSet;
    StringSet sent_symbols_;

    static QuickFAST::template_id_t TID_TRADE_MESSAGE;
    static QuickFAST::template_id_t TID_DEPTH_MESSAGE;

    void set_sequence_num(QuickFAST::Messages::FieldSet& message);

    void on_send(WorkingBufferPtr wb,
                 const boost::system::error_code& error,
                 std::size_t bytes_transferred);
  };

  typedef boost::shared_ptr<DepthFeedSession> SessionPtr;

  class DepthFeedConnection : boost::noncopyable {
  public:
    DepthFeedConnection(int argc, const char* argv[]);
    ~DepthFeedConnection();

    void connect();
    void accept();
    void run();

    void set_message_handler(MessageHandler msg_handler);
    void set_reset_handler(ResetHandler reset_handler);

    BufferPtr        reserve_recv_buffer();
    WorkingBufferPtr reserve_send_buffer();

    void send_buffer(WorkingBufferPtr& buf);
                     
    // Send a trade messsage to all clients
    void send_trade(QuickFAST::Messages::FieldSet& message);

    // Send an incremental update
    //   return true if all sessions could handle an incremental update
    bool send_incr_update(const std::string& symbol, 
                          QuickFAST::Messages::FieldSet& message);

    // Send a full update to those which have not yet received for this symbol
    void send_full_update(const std::string& symbol, 
                          QuickFAST::Messages::FieldSet& message);

    void on_connect(const boost::system::error_code& error);
    void on_accept(SessionPtr session,
                   const boost::system::error_code& error);
    void on_receive(BufferPtr bp,
                    const boost::system::error_code& error,
                    std::size_t bytes_transferred);
    void on_send(WorkingBufferPtr wb,
                 const boost::system::error_code& error,
                 std::size_t bytes_transferred);

  private:
    typedef std::deque<BufferPtr> Buffers;
    typedef std::vector<SessionPtr> Sessions;
    bool connected_;
    MessageHandler msg_handler_;
    ResetHandler reset_handler_;
    QuickFAST::Codecs::TemplateRegistryPtr templates_;

    Buffers        unused_recv_buffers_;
    WorkingBuffers unused_send_buffers_;
    Sessions sessions_;
    boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    boost::asio::io_service ios_;
    boost::asio::ip::tcp::socket socket_;
    boost::shared_ptr<boost::asio::io_service::work> work_ptr_;

    void issue_read();
  };
} } // End namespace

#endif