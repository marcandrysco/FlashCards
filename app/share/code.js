(function() {
  "use strict";

  /* App namespace */
  window.App = new Object();

  /* mode */
  window.App.mode = "none";

  /* retrieve the next card */
  window.App.next = function() {
    Req.get(location.pathname + "/rand", function(resp) {
      var json = JSON.parse(resp);
      var card = Gui.div("card");
      var list = [ "eng", "rom", "hir", "kanji", "audio" ];
      var show, actions, audio = null;

      var mk = function(el) {
        var div;
        if(el != "audio") {
          div = Gui.div(el, Gui.text(json[el]));
        } else {
          audio = Gui.tag("audio");
          audio.src = "/mp3/" + json.audio;
          var play = Gui.tag("button", "play", Gui.text("Play"));
          play.addEventListener("click", function() { audio.play(); });
          div = Gui.div("audio", [audio, play]);
        }
        return div;
      };

      var query = mk(App.mode);
      query.classList.add("prompt");
      card.appendChild(query);

      if(App.mode == "audio") { audio.play(); }

      list.map(mk).forEach(function(el) {
        el.classList.add("hidden");
        card.appendChild(el);
      });

      actions = Gui.div("actions");
      card.appendChild(actions);

      actions.appendChild(show = Gui.button("show", "Show", function() {
        var list = card.getElementsByClassName("hidden");
        for(var i = list.length - 1; i >= 0; i--) {
          list.item(i).classList.remove("hidden");
        }

        actions.insertBefore(Gui.button("good", "Good", function() {
          Req.get(location.pathname + "/inc/" + json.id, function(resp) {
            App.next();
          });
        }), show);

        actions.insertBefore(Gui.div("sep"), show);

        actions.insertBefore(Gui.button("okay", "Okay", function() {
          Req.get(location.pathname + "/reset/" + json.id, function(resp) {
            App.next();
          });
        }), show);

        actions.insertBefore(Gui.div("sep"), show);

        actions.insertBefore(Gui.button("bad", "Bad", function() {
          Req.get(location.pathname + "/dec/" + json.id, function(resp) {
            App.next();
          });
        }), show);

        actions.insertBefore(Gui.div("sep"), show);

        actions.insertBefore(Gui.button("zero", "Zero", function() {
          Req.get(location.pathname + "/zero/" + json.id, function(resp) {
            App.next();
          });
        }), show);

        if(App.mode != "audio") { audio.play(); }

        actions.removeChild(show);
        card.removeChild(query);
      }));

      actions.appendChild(Gui.div("space"));

      actions.appendChild(Gui.button("skip", "Skip", function() {
        App.next();
      }));

      Gui.replace(Gui.byid("page"), card);
    });
  };

  window.addEventListener("load", function() {
    switch(location.pathname.split('/').pop()) {
    case "eng": App.mode = "eng"; break;
    case "audio": App.mode = "audio"; break;
    case "hir": App.mode = "hir"; break;
    case "kanji": App.mode = "kanji"; break;
    default: Gui.replace(Gui.byid("content"), Gui.text("Invalid page.")); return;
    }

    App.next();
  });

})();
