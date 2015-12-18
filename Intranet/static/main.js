function apply1to1()
{
    var elems = $(".1to1");
    for (var i = 0; i < elems.size(); i++)
    {
        var e = $(elems[i]);
        e.css("min-height", e.css("width"));
    }
}

$(document).ready(apply1to1);
window.addEventListener("resize", apply1to1);
