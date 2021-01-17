var ServerUrl = location.protocol+'//'+location.hostname+(location.port ? ':'+location.port: '');

function onMainMenuAction()
{
    var selected = $('#mainmenu').val();
    console.log("MainMenu! " + selected);
    switch (selected)
    {
        default:
            if (onLocalCallback) onLocalCallback(selected);
        break;
        case 'createtx':
        case 'createAddress':
        case 'unspentTxOuts':
            window.location.assign(ServerUrl + '/' + selected);
        break;
    }
}