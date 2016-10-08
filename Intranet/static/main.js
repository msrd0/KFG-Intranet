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



function coloritems()
{
    var begin = [ 0x3A, 0xD0, 0x10 ];
    var end   = [ 0x10, 0x20, 0xE0 ];
    var b2e   = [ end[0]-begin[0], end[1]-begin[1], end[2]-begin[2] ];
    
    var rows = $('.row');
    var diag = Math.sqrt(9 + (rows.length-1) * (rows.length-1));
    
    for (var i = 0; i < rows.length; i++)
    {
        var row = $(rows[i]);
        var cells = row.children(".item");
        
        for (var j = 0; j < cells.length; j++)
        {
            var diff = Math.sqrt(i*i + j*j) / diag;
            var color = [ begin[0], begin[1], begin[2] ];
            for (var a = 0; a < 3; a++)
                color[a] += diff * b2e[a];
            
            $(cells[j]).children(".1to1").css("background-color", "rgb(" + (color[0]|0) + "," + (color[1]|0) + "," + (color[2]|0) + ")");
        }
    }
}
