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
        
            Assets assets = new Assets();
        
            if (string.IsNullOrEmpty(title.TitleName) || string.IsNullOrEmpty(title.TitleID))
            {
                assets.LargeImageText = "LiveArea";
                assets.LargeImageKey = "playstation_app_icon"; // https://upload.wikimedia.org/wikipedia/commons/9/91/PlayStation_App_Icon.jpg
                presence.Details = "In the LiveArea";
            }
            else
            {
                assets.LargeImageText = title.TitleName;
                assets.LargeImageKey = title.TitleID.ToLower();
                presence.Details = title.TitleName;
            }
        
            presence.Assets = assets;
            presence.Timestamps = time;
        
            return presence;
        }
    }
}
