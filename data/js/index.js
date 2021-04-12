$(document).ready(function () {

    if ($('#inline').length) {
        $('#inline').minicolors({
            inline: $('#inline').attr('data-inline') === 'true',
            theme: 'bootstrap'
        });
    }

    if ($('#scoreboardcolorLeft').length) {
        $('#scoreboardcolorLeft').minicolors({
            inline: $('#scoreboardcolorLeft').attr('data-inline') === 'true',
            theme: 'bootstrap'
        });
    }

    if ($('#scoreboardcolorRight').length) {
        $('#scoreboardcolorRight').minicolors({
            inline: $('#scoreboardcolorRight').attr('data-inline') === 'true',
            theme: 'bootstrap'
        });
    }


    if ($('#currentDateTime').length) {
        window.setInterval(showDateTime, 900);
    }
});

function showDateTime() {
    console.log(1);
    var currentDateTime = $('#currentDateTime').get(0);
    currentDateTime.innerHTML = moment().format('MMMM DD YYYY HH:mm:ss');
}

function setcolor() {
    var color = $('#inline').minicolors('rgbObject');
    console.log(color);
    $.post("/color", color);
}

function setBrightness() {
    var brightness = $('#rangeBrightness').val();
    console.log(brightness);
    $.post("/brightness", { brightness: brightness });
}

function setDateTime() {
    var d = moment().format('MMMM DD YYYY');
    var t = moment().format('HH:mm:ss');
    var datetime = { date: d, time: t };
    console.log(datetime);
    $.post("/setdate", datetime);
}

function startCountdown() {
    var color = $('#inline').minicolors('rgbObject');
    var h = $('#hours').val();
    var m = $('#minutes').val();
    var s = $('#seconds').val();

    if (isNaN(h) || isNaN(m) || isNaN(s)) {
        $('#validNrs').show();
        return;
    } else {
        $('#validNrs').hide();
    }

    var ms = (h * 60 * 60 * 1000) + (m * 60 * 1000) + (s * 1000);

    var countdown = { ms: ms, r: color.r, g: color.g, b: color.b };
    console.log(countdown);

    $.post("/countdown", countdown);
}

function showTemperature() {
    var c = $('#temperatureCorrection').val();
    var d = $('#temperatureDisplay').val();
    var temperature = { correction: c, symbol: d };
    $.post("/temperature", temperature);
}

function showClock() {
    $.post("/clock");
}

function showScoreboard() {
    var colorLeft = $('#scoreboardcolorLeft').minicolors('rgbObject');
    var colorRight = $('#scoreboardcolorRight').minicolors('rgbObject');

    var left = $('#scoreboardLeft').val();
    var right = $('#scoreboardRight').val();

    if (isNaN(left) || isNaN(right)) {
        $('#validNrs').show();
        return;
    } else {
        $('#validNrs').hide();
    }

    var scoreboard = {
        left: left,
        right: right,
        rl: colorLeft.r,
        gl: colorLeft.g,
        bl: colorLeft.b,
        rr: colorRight.r,
        gr: colorRight.g,
        br: colorRight.b
    };

    $.post("/scoreboard", scoreboard);
}

function upleft() {
    var left = $('#scoreboardLeft').val();
    if (left < 99 && !isNaN(left)) {
        var newval = parseInt(left) + 1;
        $('#scoreboardLeft').val(newval);
        showScoreboard();
    }
}

function downleft() {
    var left = $('#scoreboardLeft').val();
    if (left > 0 && !isNaN(left)) {
        var newval = parseInt(left) - 1;
        $('#scoreboardLeft').val(newval);
        showScoreboard();
    }
}

function upright() {
    var right = $('#scoreboardRight').val();
    if (right < 99 && !isNaN(right)) {
        var newval = parseInt(right) + 1;
        $('#scoreboardRight').val(newval);
        showScoreboard();
    }
}

function downright() {
    var right = $('#scoreboardRight').val();
    if (right > 0 && !isNaN(right)) {
        var newval = parseInt(right) - 1;
        $('#scoreboardRight').val(newval);
        showScoreboard();
    }
}

function updateHourFormat() {
    var hourformat = $('#hourFormat').val();
    $.post("/hourformat", { hourformat: hourformat });
}