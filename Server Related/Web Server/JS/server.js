const http = require("http");  // HTTP server Module
const fs = require("fs");  // File System Module

var cookiesContainer = ["testCookie123", "cookie2", "cookie3"];

var imageFile;
var mainPage;
var mainCSS;


fs.readFile(__dirname + "\\image.png", function (err, data) {  // __dirname is the current directory where this server js file is located
	if (err) {
		console.log("No Image File");
		process.exit(1);  // Remove this and others below if u want it to continue without the file
	}
    imageFile = data;
});
fs.readFile(__dirname + "\\template\\main.html","utf8",function (err, data) {
	if (err) {
		console.log("No Main Page File");
		process.exit(1);
	}
	mainPage = data;
});
fs.readFile(__dirname + "\\template\\main.css","utf8",function (err, data) {
	if (err) {
		console.log("No Main CSS File");
		process.exit(1);
	}
	mainCSS = data;
});



const server = http.createServer((req, res) => {
    const url = new URL(req.url, `http://${req.headers.host}`);
    const path = url.pathname;

    if (req.method === "POST") {  // Handle POST request
        let postData = "";

        // Listen for the 'data' event to collect the incoming POST data
        req.on("data", (chunk) => {
            postData += chunk;
        });

        // Listen for the 'end' event to process the complete POST data
        req.on("end", () => {
            try {
                if (path === "/test-post") {
                        console.log(postData)
                        res.writeHead(200, {"Content-Type": "text/html",});
                        res.end("Hello from server");
                    return;
                }
            } catch (error) {
                res.writeHead(400, {"Content-Type": "text/html",});
                res.end("Server Error");
                console.log("Error : ",error,"\nURL Path: ",path,"\nPOST data: ",postData);
            }
        });
    } else {  // Handle GET request
        if (path === "/") {
            res.writeHead(200, {"Content-Type": "text/html","Set-Cookie": "your-cookie=" + cookiesContainer[0],});  // Give new cookie
            res.end(mainPage);

        } else if (path === "/main.css") {
                res.writeHead(200, { "Content-Type": "text/html" });
                res.end(mainCSS);

        } else if (path === "/get-more-cookies") {
            res.writeHead(200, {"Content-Type": "text/html","Set-Cookie": ["your-cookie2=" + cookiesContainer[1],"your-cookie3=" + cookiesContainer[2]],});  // Give new cookie
            res.end(mainPage);

        } else if (path === "/lost-path") {
            if (req.headers["cookie"]) {  // Check if cookie exist, if exist, delete all of it
                var cookies = req.headers["cookie"].split(";");
                var cookieCrushFormat = [];
                for (let i = 0; i < cookies.length; i++) {
                    cookieCrushFormat.push(cookies[i].split("=")[0].trim() + "=; max-age=0");
                }
                res.setHeader("set-cookie", cookieCrushFormat);
            }
            res.writeHead(200, {"Content-Type": "text/html",});
            res.end('<script>window.location="http://"+(new URL(window.location.href).hostname)+"/"</script>');  // Redirect back to "/"

        } else if (path === "/image.png") {
            res.writeHead(200, { "Content-Type": "image/png" });
            res.end(imageFile);

        } else {
            res.writeHead(404);
            res.end("404 Not Found\n");
            
        }
    }
});

const PORT = 80;
const IP_ADDRESS = "localhost";

server.listen(PORT, IP_ADDRESS, () => {
    console.log(`Server running at http://${IP_ADDRESS}:${PORT}/`);
});
