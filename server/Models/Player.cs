using System;
using System.Net.Sockets;

namespace GameServer
{
    public class Player
    {
        public string Id { get; set; }
        public string Name { get; set; }
        public TcpClient Client { get; set; }
        public DateTime ConnectedAt { get; private set; }

        public bool IsConnected
        {
            get
            {
                if (Client == null)
                {
                    return false;
                }
                return Client.Connected;
            }
        }

        public Player()
        {
            Id = Guid.NewGuid().ToString();
            Name = "Player";
            ConnectedAt = DateTime.UtcNow;
        }
    }
}