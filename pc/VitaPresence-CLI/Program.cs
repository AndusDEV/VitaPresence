using DiscordRPC;
using Microsoft.VisualBasic;
using PresenceCommon;
using PresenceCommon.Types;
using System;
using System.Net;
using System.Net.Http;
using System.Threading;
using System.Timers;
using Timer = System.Timers.Timer;

namespace VitaPresence_CLI
{
    class Program
    {
        static Timer timer;
        private static HttpClient client;
        private static Uri BaseAddress;
        private static int updateInterval = 10;
        static string LastGame = "";
        static Timestamps time = null;
        static DiscordRpcClient rpc;

        static int Main(string[] args)
        {
            AppDomain.CurrentDomain.ProcessExit += CurrentDomain_ProcessExit;
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: VitaPresence-CLI <IP> <Client ID> <Update Interval (Optional. Default is 10 seconds.)>");
                return 1;
            }

            if (!IPAddress.TryParse(args[0], out IPAddress iPAddress))
            {
                Console.WriteLine("Invalid IP");
                return 1;
            }

            rpc = new DiscordRpcClient(args[1]);

            if (!rpc.Initialize())
            {
                Console.WriteLine("Unable to start RPC!");
                return 2;
            }

            if (args.Length > 2)
            {
                if (!int.TryParse(args[2], out updateInterval))
                {
                    Console.WriteLine("Invalid update interval!\nPlease enter time in seconds.");
                    return 3;
                }
            }

            IPEndPoint localEndPoint = new IPEndPoint(iPAddress, 0xCAFE);

            timer = new Timer()
            {
                Interval = 15000,
                Enabled = false,
            };
            timer.Elapsed += new ElapsedEventHandler(OnConnectTimeout);

            while (true)
            {
                client = new HttpClient();
                client.BaseAddress = new Uri("http://" + localEndPoint.ToString());

                timer.Enabled = true;

                try
                {
                    IAsyncResult result = client.GetAsync(BaseAddress);
                    bool success = result.AsyncWaitHandle.WaitOne(2000, true);
                    if (!success)
                    {
                        Console.WriteLine((result.AsyncState));
                        Console.WriteLine("Could not connect to Server! Retrying...");
                        client.Dispose();
                    }
                    else
                    {
                        timer.Enabled = false;
                        DataListen();
                        client.Dispose();
                    }
                    Thread.Sleep(updateInterval * 1000); // wait before another connect
                }
                catch (Exception e)
                {
                    client.Dispose();
                    if (rpc != null && !rpc.IsDisposed) rpc.ClearPresence();
                }
            }
        }

        private static void DataListen()
        {
            try
            {
                string str = client.GetStringAsync(BaseAddress).Result;

                Console.WriteLine("'" + str + "' received...");

                Title title = new Title(str);

                if (LastGame != title.TitleID)
                {
                    time = Timestamps.Now;
                }
                if ((rpc != null && rpc.CurrentPresence == null) || LastGame != title.TitleID)
                {
                    rpc.SetPresence(Utils.CreateDiscordPresence(title, time));

                    LastGame = title.TitleID;
                }
            }
            catch (Exception e)
            {
                if (rpc != null && !rpc.IsDisposed) rpc.ClearPresence();
                client.Dispose();
                return;
            }

        }

        private static void OnConnectTimeout(object sender, ElapsedEventArgs e)
        {
            LastGame = "";
            time = null;
        }

        private static void CurrentDomain_ProcessExit(object sender, EventArgs e)
        {
            if (client != null)
                client.Dispose();

            if (rpc != null && !rpc.IsDisposed)
                rpc.Dispose();
        }
    }
}
