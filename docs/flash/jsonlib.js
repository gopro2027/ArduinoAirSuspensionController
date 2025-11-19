// Javascript library because of the limitations of the flash. I use this to generate the manifest files
// See this link as an alternative: https://github.com/esphome/esp-web-tools/issues/498#issuecomment-2637621911


// if you take my api key I will find you and tickle your toes
apikey = "ae3f8174-4803-11f0-8d3f-0242ac110008"

function getJson(id) {
    const request = new XMLHttpRequest();
	request.open("GET", "https://json.extendsclass.com/bin/"+id, false);
	// request.onreadystatechange = () => {
	// 	//alert(request.responseText);
    //     after(request.responseText);
	// };
	request.send();
    if (request.status === 200) {
        console.log(request.responseText); // Access the response text
        return request.responseText;
    } else {
        console.error("Request failed with status:", request.status);
        return "error";
    }
}

function createJson(obj) {
    const request = new XMLHttpRequest();
	request.open("POST", "https://json.extendsclass.com/bin", false);
	request.setRequestHeader("Api-key", apikey);
	// request.setRequestHeader("Security-key", "Your security key");
	// request.setRequestHeader("Private", "true");
	// request.onreadystatechange = () => {
    //     //alert(request.responseText);
    //     after(request.responseText);
	// };
	request.send(JSON.stringify(obj));
    if (request.status === 200) {
        console.log(request.responseText); // Access the response text
        return request.responseText;
    } else {
        console.error("Request failed with status:", request.status);
        return "error";
    }
}

function deleteJson(id) {
    const request = new XMLHttpRequest();
	request.open("DELETE", "https://json.extendsclass.com/bin/"+id, true);
	// request.setRequestHeader("security-key", "Your security key");
	request.onreadystatechange = () => {
	};
	request.send();
}

function generateManifestFileLink(manifestObject) {
    createJson();

    setTimeout(deleteJson(id), 10000)
}