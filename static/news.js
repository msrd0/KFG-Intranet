function news_edit(id)
{
    var elem = $('#news' + id + 'text');
    if (elem.hasClass("news_editing"))
        return;
    elem.addClass("news_editing");
    var text = elem.html();
    var html = "";
    html += '<form action="' + prepend + 'editnews" method="POST" enctype="multipart/form-data">';
    html +=   '<input type="hidden" name="id" value="' + id + '">';
    html +=   '<input type="hidden" name="redir" value="' + window.location.href + '">';
    html +=   '<textarea name="text" style="width: 95%" rows=5>';
    html +=     text;
    html +=   '</textarea>';
    html +=   '<input type="submit" value="Speichern">';
    html += '</form>';
    elem.html(html);
}

function news_delete(id)
{
    if (!confirm("Wirklich löschen?"))
        return;
    $.get(prepend + 'deletenews?id=' + id, function(data) {
        window.location.reload();
    });
}
