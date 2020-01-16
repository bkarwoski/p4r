let downloading_image = new Image();

function byId(id) {
    return document.getElementById(id);
}

function update() {
    if (document.hidden) {
        return;
    }
    downloading_image.src = "http://localhost:8000/image.bmp#" + new Date().getTime();
    downloading_image.onload = function() {
        byId("image").src = downloading_image.src;
    };
}

function init() {
    update();
    setInterval(update, 40);
}

window.onload = init;
