using DiscordRPC;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using PresenceCommon.Types;
using System;
using System.Diagnostics;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;

namespace PresenceCommon
{
    public static class Utils
    {
        public static async Task<IPAddress> GetExternalIpAddress()
        {
            var externalIpString = (await new HttpClient().GetStringAsync("http://icanhazip.com"))
                .Replace("\\r\\n", "").Replace("\\n", "").Trim();
            if (!IPAddress.TryParse(externalIpString, out var ipAddress)) return null;
            return ipAddress;
        }

        public static RichPresence CreateDiscordPresence(Title title, Timestamps time, string state = "")
        {
            RichPresence presence = new RichPresence()
            {
                State = state
            };

            Assets assets = new Assets {};

            HttpClient client = new HttpClient();

            if (string.IsNullOrEmpty(title.TitleName) || string.IsNullOrEmpty(title.TitleID))
            {
                assets.LargeImageText = "LiveArea";
                presence.Details = "In the LiveArea";
            }
            else
            {
                assets.LargeImageText = title.TitleName;
                presence.Details = $"{title.TitleName}";
            }
            try
            {
                IPEndPoint imageEndPoint = new IPEndPoint(GetExternalIpAddress().Result, 0xCAFE);
                string image = $"http://{imageEndPoint}/{title.TitleID}.png";
                client.BaseAddress = new Uri(image);
                var response = client.GetAsync(client.BaseAddress).Result;
                assets.LargeImageKey = image;
            }
            catch (Exception)
            {
                assets.LargeImageKey = "playstation_app_icon";
            }

            presence.Assets = assets;
            presence.Timestamps = time;
            
            client.Dispose();
            
            return presence;
        }
    }
}
