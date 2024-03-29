
var storedHash = window.localStorage.getItem("storedHash");
var rememberPassword = false;
var hashedPass;
var passwordInput = document.getElementById("password");
passwordInput.focus();

if(storedHash && storedHash.trim().length) {
    checkPass();
}

function login() {
    checkPass();
}

function checkPass(userEntered) {
    var xhr = new XMLHttpRequest();
    xhr.open('POST', "/auth", true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.onreadystatechange = function () {
        if (xhr.readyState === 4) {
            var status = xhr.status;
            if (status !== 200) {
                showAlertWindow("Error", xhr.statusText);
                if(status == 401 && storedHash)
                    window.localStorage.removeItem("storedHash");
            } else {
                if(rememberPassword)
                    window.localStorage.setItem("storedHash", hashedPass);
                //var sessionID = JSON.parse(xhr.response).sessionID;
                //window.sessionStorage.setItem("sessionID", sessionID);
                document.removeEventListener("keypress", onEnter);
                window.location.replace(window.location.origin);
            }
        }
    }
    xhr.onerror = function () {
        onSaveFail(xhr.statusText);
    };
    if(storedHash && !userEntered) {
        hashedPass = storedHash;
        xhr.send("{\"hashedPass\":\""+storedHash+"\", \"remember\":\"true\"}");
    } else {
        hashedPass = encrypt();
        if(hashedPass) {
            xhr.send("{\"hashedPass\":\""+hashedPass+"\", \"remember\":\""+rememberPassword+"\"}");
        } else
            showAlertWindow("Error", "Invalid password");
    }
}

function encrypt() {
    const pass = passwordInput.value;
    if(pass.trim().length) {
        return keccak512(pass);
    }
    return undefined;
}

function rememberCheckbox(checkbox) {
    rememberPassword = checkbox.checked;
}

document.addEventListener('keypress', onEnter)

function onEnter(e) {
    if (e.key === 'Enter' && passwordInput.value.trim().length)
        checkPass(true);
}