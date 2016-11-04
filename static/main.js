function apply1to1()
{
    var elems = $(".1to1");
    for (var i = 0; i < elems.size(); i++)
    {
        var e = $(elems[i]);
        var width = e.css("width");
        e.css("min-height", width);
        e.css("max-height", width);
    }
}

$(document).ready(apply1to1);
window.addEventListener("resize", apply1to1);



function coloritems()
{
    var bgbegin = [ 0x3A, 0xD0, 0x10 ];
    var bgend   = [ 0x10, 0x20, 0xE0 ];
    var bgb2e   = [ bgend[0]-bgbegin[0], bgend[1]-bgbegin[1], bgend[2]-bgbegin[2] ];
    
    var fgbegin = [ 0x80, 0x40, 0x10 ];
    var fgend   = [ 0xFF, 0x40, 0x00 ];
    var fgb2e   = [ fgend[0]-fgbegin[0], fgend[1]-fgbegin[1], fgend[2]-fgbegin[2] ];
    
    var rows = $('.row');
    var diag = Math.sqrt(9 + (rows.length-1) * (rows.length-1));
    
    for (var i = 0; i < rows.length; i++)
    {
        var row = $(rows[i]);
        var cells = row.children(".item");
        
        for (var j = 0; j < cells.length; j++)
        {
            var diff = Math.sqrt(i*i + j*j) / diag;
            var bgcolor = [ bgbegin[0], bgbegin[1], bgbegin[2] ];
            var fgcolor = [ fgbegin[0], fgbegin[1], fgbegin[2] ];
            for (var a = 0; a < 3; a++)
            {
                bgcolor[a] += diff * bgb2e[a];
                fgcolor[a] += diff * fgb2e[a];
            }
            
            $(cells[j]).children(".1to1").css("background-color", "rgb(" + (bgcolor[0]|0) + "," + (bgcolor[1]|0) + "," + (bgcolor[2]|0) + ")");
            $(cells[j]).children(".1to1").css("color"           , "rgb(" + (fgcolor[0]|0) + "," + (fgcolor[1]|0) + "," + (fgcolor[2]|0) + ")");
        }
    }
}
