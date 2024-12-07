using HtmlAgilityPack;
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace PresenceCommon.Types
{
    public class Title
    {
        public string TitleName { get; }
        public string TitleID { get; }

        public Title(string str)
        {
            var htmldoc = new HtmlDocument();
            htmldoc.LoadHtml(str);
            var p = htmldoc.DocumentNode.SelectNodes("//p");
            TitleName = p[0].InnerText;
            TitleID = p[1].InnerText;
        }
    }
}
