function includeHTML(callback) {
  var z, i, elmnt, file, xhttp;
  /* Loop through a collection of all HTML elements: */
  z = document.getElementsByTagName("*");
  for (i = 0; i < z.length; i++) {
    elmnt = z[i];
    /*search for elements with a certain atrribute:*/
    file = elmnt.getAttribute("include");
    if (file) {
      /* Make an HTTP request using the attribute value as the file name: */
      xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
          if (this.status == 200) { elmnt.innerHTML = this.responseText; }
          if (this.status == 404) { elmnt.innerHTML = "Page not found."; }
          /* Remove the attribute, and call this function once more: */
          elmnt.removeAttribute("include");
          includeHTML(callback);
        }
      }
      xhttp.open("GET", file, true);
      xhttp.send();
      /* Exit the function: */
      return;
    }
  }
  callback();
}

function setNavItemActive() {
  var elmnts, i, elmnt, dropdownElmnts, dropdownElmnt;
  elmnts = document.getElementsByClassName("nav-item");
  for (i = 0; i < elmnts.length; i++) {
    elmnt = elmnts[i];
    switch (String(elmnt.className)) {
      case "nav-item":
        if (window.location.pathname.endsWith(elmnt.firstChild.getAttribute("href"))) {
          elmnt.className = "nav-item active";
        }
        break;
      case "nav-item dropdown":
        dropdownElmnts = elmnt.getElementsByClassName("dropdown-item")
        for (i = 0; i < dropdownElmnts.length; i++) {
          dropdownElmnt = dropdownElmnts[i];
          if (window.location.pathname.endsWith(dropdownElmnt.getAttribute("href"))) {
            elmnt.className = "nav-item active";
          }
        }
        break;
    }
  }
}
