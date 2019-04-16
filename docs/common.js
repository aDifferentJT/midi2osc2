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
  var z, i, elmnt;
  z = document.getElementsByClassName("nav-item");
  console.log(z.length);
  for (i = 0; i < z.length; i++) {
    elmnt = z[i];
    var path_elements = window.location.pathname.split("/");
    var file_name = path_elements[path_elements.length - 1]
    if (elmnt.firstChild.getAttribute("href") == file_name) {
      elmnt.className = "nav-item active";
    }
  }
}
