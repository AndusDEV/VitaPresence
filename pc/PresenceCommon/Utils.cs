using DiscordRPC;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using PresenceCommon.Types;
using System;
using System.Net.Http;
using System.Threading.Tasks;

namespace PresenceCommon
{
    public static class Utils
    {

        static async Task<string> getImage(string URL)
        {
            HttpClient client = new HttpClient();
            string response = await client.GetStringAsync(URL);
            JObject json = JObject.Parse(response);
            return json["images"][2]["url"].ToString();
        }

        public static RichPresence CreateDiscordPresence(Title title, Timestamps time, string state = "")
        {
            RichPresence presence = new RichPresence()
            {
                State = state
            };

            Assets assets = new Assets {};

            if (title.Index == 0)
            {
                assets.LargeImageText = "LiveArea";
                //assets.LargeImageKey = $"0{0x0100000000001000:x}";
                presence.Details = "In the LiveArea";
            }
            else
            {
                assets.LargeImageText = title.TitleName;
                //assets.LargeImageKey = $"0{title.TitleID:x}";
                presence.Details = $"{title.TitleName}";
            }
            try
            {
                // TODO: only works for US region games for now.
                string image = getImage("https://store.playstation.com/store/api/chihiro/00_09_000/container/US/en/999/" + title.ContentID).Result;
                assets.LargeImageKey = image;
            }
            catch 
            {
                assets.LargeImageKey = "https://upload.wikimedia.org/wikipedia/commons/9/91/PlayStation_App_Icon.jpg";
            }
            presence.Assets = assets;
            presence.Timestamps = time;
            
            return presence;
        }
    }
}
