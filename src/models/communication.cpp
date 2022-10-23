#include "./communication.hpp"

Communication::Communication(const otf2::chrono::duration &start, const otf2::chrono::duration &end,
                             otf2::definition::location sender, otf2::definition::location receiver)
                             : start(start), end(end), sender(std::move(sender)), receiver(std::move(receiver)) {}

