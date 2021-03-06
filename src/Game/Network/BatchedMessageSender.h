
#pragma once

#include "NetworkSerialize.h"
#include <queue>

namespace game
{
    struct NetworkMessage;

    class BatchedMessageSender
    {
    public:

        BatchedMessageSender(const network::Address& address, std::queue<NetworkMessage>& out_messages)
            : m_out_messages(out_messages)
        {
            PrepareMessageBuffer(m_network_message.payload);
            m_network_message.address = address;
        }

        ~BatchedMessageSender()
        {
            if(!m_network_message.payload.empty())
                m_out_messages.push(m_network_message);
        }

        template <typename T>
        void SendMessage(const T& message)
        {
            const bool success = SerializeMessageToBuffer(message, m_network_message.payload);
            if(!success)
            {
                m_out_messages.push(m_network_message);

                m_network_message.payload.clear();
                PrepareMessageBuffer(m_network_message.payload);
                SerializeMessageToBuffer(message, m_network_message.payload);
            }
        }

    private:

        std::queue<NetworkMessage>& m_out_messages;
        NetworkMessage m_network_message;
    };
}
