var ServerUrl = location.protocol+'//'+location.hostname+(location.port ? ':'+location.port: '');

function RequestShutdown()
{
    var shutdownHttp = new XMLHttpRequest();
    shutdownHttp.open("GET", `${ServerUrl}/rest/shutdown`);
    shutdownHttp.send();
    shutdownHttp.onreadystatechange = (e) => { };
}

function RequestMiningStart(readyCallback)
{
    Http = new XMLHttpRequest();
    Http.open("GET", ServerUrl+"/rest/startMining");
    Http.onreadystatechange = readyCallback;
    return Http;
}

function RequestMiningStop(readyCallback)
{
    Http = new XMLHttpRequest();
    Http.open("GET", ServerUrl+"/rest/stopMining");
    Http.onreadystatechange = readyCallback;
    Http.send();
}