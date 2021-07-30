#ifdef __unix__
#   include <fcntl.h>
#   include <sys/socket.h>
#endif

#include <cassert>

#include "remote-gui.hh"
#include <messages.hh>

namespace clap {

   bool RemoteGui::spawn() {
      assert(child_ == -1);

#ifdef __unix__
      assert(!channel_);

      /* create a socket pair */
      int sockets[2];
      if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets)) {
         return false;
      }

      child_ = ::fork();
      if (child_ == -1) {
         ::close(sockets[0]);
         ::close(sockets[1]);
         return false;
      }

      if (child_ == 0) {
         // Child
         ::close(sockets[0]);
         char socketStr[16];
         ::snprintf(socketStr, sizeof(socketStr), "%d", sockets[1]);
         ::execl("clap-gui", "--socket", socketStr);
         std::terminate();
      } else {
         // Parent
         ::close(sockets[1]);
      }

      channel_.reset(new RemoteChannel(
         [this](const RemoteChannel::Message &msg) { onMessage(msg); }, *this, sockets[0], true));

      return true;
#else
      return false;
#endif
   }

   bool RemoteGui::size(uint32_t *width, uint32_t *height) noexcept {
      channel_->sendMessageSync(
         RemoteChannel::Message(messages::SizeRequest{}, channel_->computeNextCookie()),
         [width, height](const RemoteChannel::Message &m) {
            auto &response = m.get<messages::SizeResponse>();
            *width = response.width;
            *height = response.height;
         });

      return true;
   }

   void RemoteGui::setScale(double scale) noexcept {
      channel_->sendMessageAsync(
         RemoteChannel::Message(messages::SetScaleRequest{scale}, channel_->computeNextCookie()));
   }

} // namespace clap