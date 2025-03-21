function validateForm(event) {
    if (event) event.preventDefault();
    function isValidIP(ip) {
        return /^((25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[0-1]?[0-9][0-9]?)$/.test(ip);
    }
    function isValidPort(port) {
        return /^[1-9][0-9]{0,4}$/.test(port) && port > 0 && port <= 65535;
    }
    function isValidUrl(url) {
        try {
            let parsedUrl = new URL(url);
            return (parsedUrl.protocol === 'http:' || parsedUrl.protocol === 'https:') && parsedUrl.hostname.length > 0;
        } catch (e) {
            return false;
        }
    }
    
    let ip = document.getElementById('ip').value;
    let gateway = document.getElementById('gateway').value;
    let subnetMask = document.getElementById('subnetMask').value;
    let tcpIp = document.getElementById('tcpIp').value;
    let tcpPort = document.getElementById('tcpPort').value;
    let httpUrl = document.getElementById('httpUrl').value;
    let adminPassword = document.getElementById('adminPassword').value;

    // Device network settings validation
    if (!isValidIP(ip) || !isValidIP(gateway) || !isValidIP(subnetMask)) {
        alert('Invalid IP Address, Gateway, or Subnet Mask!');
        return false;
    }

    // Tcp IP and Port validation
    if (!isValidIP(tcpIp)) {
        if (document.getElementById('tcpEnabled').checked) {
            alert('Invalid TCP IP address!');
            return false;
        }
    }
    if (!isValidPort(tcpPort)) {
        if (document.getElementById('tcpEnabled').checked) {
            alert('Invalid TCP Port! Must be between 1-65535.');
            return false;
        }
    }

    // HTTP URL validation
    if (!isValidUrl(httpUrl)) {
        if (document.getElementById('httpEnabled').checked) {
            alert('Invalid HTTP URL!');
            return false;
        }
    }
    
    // Сompanion IP and Port validation
    if (!isValidIP(tcpIp)) {
        if (document.getElementById('tcpEnabled').checked) {
            alert('Invalid TCP IP address!');
            return false;
        }
    }
    if (!isValidPort(tcpPort)) {
        if (document.getElementById('tcpEnabled').checked) {
            alert('Invalid TCP Port! Must be between 1-65535.');
            return false;
        }
    }

    // Admin password length validation
    if (adminPassword.length > 32) {
        alert('Admin password too long! (Max 32 characters)');
        return false;
    }

    sendJson();
    return false;
}

// Post func collect only the enabled data and send it (eg if tcp not enabled, the result wont send any tcp related data)
function sendJson() {
    
    let data = {
        ip: document.getElementById('ip').value,
        gateway: document.getElementById('gateway').value,
        subnetMask: document.getElementById('subnetMask').value,
        companionMode: document.getElementById('companionEnabled').value,
        tcpEnabled:document.getElementById('tcpEnabled').checked,
        httpEnabled:document.getElementById('httpEnabled').checked,
        serialEnabled: document.getElementById('serialEnabled').checked
    };

    // Add tcpData if enabled
    if (data.tcpEnabled) {
        data.tcpEnabled = true;
        data.tcpIp = document.getElementById('tcpIp').value;
        data.tcpPort = parseInt(document.getElementById('tcpPort').value) || 0;
        data.tcpSecure = document.getElementById('tcpSecure').checked;
        if (data.tcpSecure) {
            data.tcpUser = document.getElementById('tcpUser').value;
            data.tcpPassword = document.getElementById('tcpPassword').value;
        }
    }

    // Add HttpData if enabled
    if (data.httpEnabled) {
        data.httpEnabled = true;
        data.httpUrl = document.getElementById('httpUrl').value;
        data.httpSecure = document.getElementById('httpSecure').checked;
        if (data.httpSecure) {
            data.httpUser = document.getElementById('httpUser').value;
            data.httpPassword = document.getElementById('httpPassword').value;
        }
    }

    // Add HttpData if enabled
    if (data.companionMode) {
        data.companionMode = true;
        data.companionIp = document.getElementById('companionIp').value;
        data.companionPort = document.getElementById('companionPort').value;
        
        // Force disabling all other protocols
        data.httpEnabled = false;
        data.tcpEnabled = false;
        data.serialEnabled = false;
    }

    let adminPassword = document.getElementById('adminPassword').value;
    
    // Send password only if user set some data there
    if (adminPassword.length > 0) {
        data.adminPassword = adminPassword;
    }

    fetch('/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    })
        .then(response => {
            if (response.ok) {
                alert('Configuration Saved Successfully!');
                location.reload();
            } else {
                alert('Error saving configuration!');
            }
        });
}

// Disables/Enables secure props based on secure enabled checkbox(used for both tcp/http).
function toggleSecureMode(type) {
    let secureCheckbox = document.getElementById(type + 'Secure');
    let userField = document.getElementById(type + 'User');
    let passwordField = document.getElementById(type + 'Password');
    if (secureCheckbox.checked) {
        userField.disabled = false;
        passwordField.disabled = false;
    } else {
        userField.disabled = true;
        passwordField.disabled = true;
    }
}

// Disables/Enables config props based on tcp enabled checkbox.
function toggleTcpSettings() {
    let tcpEnabledCheckbox = document.getElementById('tcpEnabled');
    let tcpIp = document.getElementById('tcpIp');
    let port = document.getElementById('tcpPort');
    let tcpSecureCheckbox = document.getElementById('tcpSecure');
    if (tcpEnabledCheckbox.checked) {
        tcpIp.disabled = false;
        port.disabled = false;
        tcpSecureCheckbox.disabled = false;
        toggleSecureMode('tcp');
    } else {
        tcpIp.disabled = true;
        port.disabled = true;
        tcpSecureCheckbox.disabled = true;
        document.getElementById('tcpUser').disabled = true;
        document.getElementById('tcpPassword').disabled = true;
    }
}

// Disables/Enables config props based on http enabled checkbox.
function toggleHttpSettings() {
    let httpEnabledCheckbox = document.getElementById('httpEnabled');
    let httpUrl = document.getElementById('httpUrl');
    let httpSecureCheckbox = document.getElementById('httpSecure');
    if (httpEnabledCheckbox.checked) {
        httpUrl.disabled = false;
        httpSecureCheckbox.disabled = false;
        toggleSecureMode('http');
    } else {
        httpUrl.disabled = true;
        httpSecureCheckbox.disabled = true;
        document.getElementById('httpUser').disabled = true;
        document.getElementById('httpPassword').disabled = true;
    }
}

// Disables/Enables config props based on http enabled checkbox.
function toggleCompanionMode() {
    let companionEnabledCheckbox = document.getElementById('companionEnabled');

    if (companionEnabledCheckbox.checked) {
        document.getElementById("manual-settings-block").classList.add("hidden");
    } else {
        document.getElementById("manual-settings-block").classList.remove("hidden");
        toggleTcpSettings();
        toggleHttpSettings();
    }
}

window.onload = function () {
    toggleTcpSettings();
    toggleHttpSettings();
    toggleCompanionMode();
};