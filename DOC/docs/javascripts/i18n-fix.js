// Fix the language switcher when the site is served from a sub-path.
// Material writes the switcher hrefs as site-root-absolute ("/zh/") using site_url.
// The theme logo's href is relative ("."), so it always resolves to the REAL site root,
// wherever the build happens to be deployed. Use it as the base and rewrite the links.
// Belt-and-braces only: the proper fix is a correct site_url at build time.
document$.subscribe(function () {
  var logo = document.querySelector(".md-header__button.md-logo");
  if (!logo) return;
  var base = logo.href;
  document.querySelectorAll(".md-select__link").forEach(function (a) {
    var h = a.getAttribute("href");
    if (h && h.charAt(0) === "/") {
      a.setAttribute("href", new URL(h.slice(1), base).href);
    }
  });
});
