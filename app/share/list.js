(function() {
  "use strict";

  window.addEventListener("load", function() {
    Req.get(location.pathname.substr(0,location.pathname.lastIndexOf("/")) + "/all", function(resp) {
      var json = JSON.parse(resp);

      for(var i = 0; i < json.length; i++) {
        (function() {
          var score, card = Gui.div("card");

          switch(json[i].score) {
          case 0: score = "new"; break;
          case 1: score = "started"; break;
          case 2: score = "recognize"; break;
          case 3: score = "recall"; break;
          case 4: score = "learned"; break;
          case 5: score = "mastered"; break;
          }

          var dist, tm = json[i].time / 1000000 - Date.now() / 1000;
          if(tm < 0) {
            dist = "now";
            tm = "now";
          } else if(tm < 60) {
            dist = "sec";
            tm = Math.floor(tm) + " sec";
          } else if(tm < 60*60) {
            dist = "min";
            tm = Math.floor(tm / 60) + "min " + Math.floor(tm % 60) + " sec";
          } else if(tm < 24*60*60) {
            dist = "hour";
            tm = Math.floor(tm / (60*60)) + "hr " + Math.floor((tm / 60) % 60) + " min";
          } else {
            dist = (tm >= 7*24*60*60) ? "week" : "day";
            tm = Math.floor(tm / (24*60*60)) + "day " + Math.floor((tm / (60*60)) % 24) + " hr";
          }

          card.appendChild(Gui.div("score " + score, Gui.text(json[i].score + ": " + score)));
          card.appendChild(Gui.div("time " + dist, Gui.text(tm)));
          card.appendChild(Gui.div("eng", Gui.text(json[i].eng)));
          card.appendChild(Gui.div("rom", Gui.text(json[i].rom)));
          card.appendChild(Gui.div("hir", Gui.text(json[i].hir)));
          card.appendChild(Gui.div("kanji", Gui.text(json[i].kanji)));

          if(json[i].audio != "_") {
            var audio = Gui.tag("audio");
            audio.src = "/mp3/" + json[i].audio;
            var play = Gui.tag("button", "play", Gui.text("🔊"));
            play.addEventListener("click", function() { audio.play(); });
            card.appendChild(Gui.div("audio", [audio, play]));
          }

          Gui.byid("page").appendChild(card);
        })();
      }
    });
  });
})();
