(function() {
  "use strict";

  /* App namespace */
  window.App = new Object();

  /* mode */
  window.App.mode = "none";

  /* retrieve the score name */
  window.App.score = function(num) {
    switch(num) {
    case 0: return "new"; break;
    case 1: return "started"; break;
    case 2: return "recognize"; break;
    case 3: return "recall"; break;
    case 4: return "learned"; break;
    case 5: return "mastered"; break;
    }
    throw "Inavlid score";
  };

  /* retrieve the next card */
  window.App.next = function() {
    Req.get(location.pathname + "/rand", function(resp) {
      var json = JSON.parse(resp);
      var card = Gui.div("card");
      var list = [ "eng", "rom", "hir", "kanji", "audio" ];
      var space, show, actions, audio = null;

      var mk = function(el) {
        var div;
        if(el != "audio") {
          div = Gui.div(el, Gui.text(json[el]));
        } else {
          if(json.audio != "_") {
            audio = Gui.tag("audio");
            audio.src = "/mp3/" + json.audio;
            var play = Gui.tag("button", "play", Gui.text("Play"));
            play.addEventListener("click", function() { audio.play(); });
            div = Gui.div("audio", [audio, play]);
          } else {
            div = Gui.div("audio", Gui.text("No audio"));
          }
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

      var notes = Gui.tag("input", "notes");
      notes.type = "text";
      notes.placeholder = "Notes";
      notes.autofocus = true;
      notes.addEventListener("keyup", function(e) {
        if(e.keyCode == 13) { show.click(); }
      });
      card.appendChild(notes);

      actions = Gui.div("actions");
      card.appendChild(actions);

      actions.appendChild(show = Gui.button("show", "Show", function() {
        var list = card.getElementsByClassName("hidden");
        for(var i = list.length - 1; i >= 0; i--) {
          list.item(i).classList.remove("hidden");
        }

        var req = function(verb) {
          return function(e) {
            Req.get(location.pathname + "/" + verb + "/" + json.id, function(resp) {
              App.next();
            });
          };
        };

        actions.insertBefore(Gui.button("good", "Good", req("inc")), show);
        actions.insertBefore(Gui.div("sep"), show);
        actions.insertBefore(Gui.button("okay", "Okay", req("reset")), show);
        actions.insertBefore(Gui.div("sep"), show);
        actions.insertBefore(Gui.button("bad", "Bad", req("bad")), show);
        actions.insertBefore(Gui.div("sep"), show);
        actions.insertBefore(Gui.button("zero", "Zero", req("zero")), show);
        space.appendChild(Gui.div("score " + App.score(json.score), Gui.text(App.score(json.score))));

        if((App.mode != "audio") && (audio != null)) { audio.play(); }

        var list = actions.getElementsByTagName("button");
        for(var i = 0; i < list.length; i++) {
          list.item(i).addEventListener("keyup", function(e) {
            if(e.code == "KeyG") { req("inc")(); }
            if(e.code == "KeyO") { req("reset")(); }
            if(e.code == "KeyB") { req("dec")(); }
            if(e.code == "KeyZ") { req("zero")(); }
            if(e.code == "KeyS") { App.next(); }
          });
        }

        actions.removeChild(show);
        card.removeChild(query);
        actions.firstChild.focus();
      }));

      actions.appendChild(space = Gui.div("space"));

      actions.appendChild(Gui.button("skip", "Skip", function() {
        App.next();
      }));

      Gui.replace(Gui.byid("page"), card);
      notes.focus();
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
