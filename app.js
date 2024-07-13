const express = require('express');
const http = require('http');
const session = require('express-session');
const multer = require('multer');
const fs = require('fs-extra');
const path = require('path');
const { Server } = require("socket.io");
const bcrypt = require('bcryptjs');

const app = express();
const PORT = 3000;
const server = http.createServer(app);
const io = new Server(server);

// Multer Configuration for handling file uploads
const fileStorageEngine = multer.diskStorage({
    destination: (req, file, cb) => cb(null, './uploads/'),
    filename: (req, file, cb) => {
        const uniqueSuffix = `${Date.now()}-${Math.round(Math.random() * 1E9)}`;
        cb(null, `${file.fieldname}-${uniqueSuffix}${path.extname(file.originalname)}`);
    }
});
const upload = multer({ storage: fileStorageEngine });

// Express Middleware Setup
app.use(express.urlencoded({ extended: true }));
app.use(express.json());
app.use(express.static('public'));
app.use('/uploads', express.static(path.join(__dirname, 'uploads')));
app.use(session({
    secret: 'your_secret_key',
    resave: false,
    saveUninitialized: true,
    cookie: {
        secure: false,  // Set to true if using HTTPS
        httpOnly: true 
    }
}));

// Directory setup for user data
const sensorDataFilePath = path.join(__dirname, 'data', 'sensordata.json');
const userDataPath = path.join(__dirname, 'data', 'users');
fs.ensureDirSync(userDataPath);
fs.ensureFileSync(sensorDataFilePath);

// EJS view engine setup
app.set('view engine', 'ejs');

// Routes for user interfaces
app.get('/', (req, res) => res.render('index'));
app.get('/signin', (req, res) => res.render('signin'));
app.get('/signup', (req, res) => res.render('signup'));
app.get('/dashboard', (req, res) => {
    if (!req.session.user) {
        return res.redirect('/signin');
    }
    res.render('dashboard', { user: req.session.user });
});

app.get('/api/sensor_data', (req, res) => {
    try {
        let data = fs.readFileSync(sensorDataFilePath, 'utf8');
        data = data.trim(); // Trim whitespace
        if (data === "") {
            data = []; 
        } else {
            data = JSON.parse(data);
        }

        // Filter data to include only the last 30 days
        const thirtyDaysAgo = new Date();
        thirtyDaysAgo.setDate(thirtyDaysAgo.getDate() - 30);
        const filteredData = data.filter(entry => new Date(entry.timestamp) >= thirtyDaysAgo);

        res.json(filteredData);
    } catch (err) {
        console.error('Failed to read or parse sensor data:', err);
        res.status(500).send('Internal Server Error: Could not read sensor data');
    }
});



app.get('/logout', (req, res) => {
    req.session.destroy(err => {
        if (err) {
            console.error('Failed to destroy the session during logout', err);
            return res.status(500).send('Could not log out, error occurred');
        }
        res.clearCookie('connect.sid');        
        res.redirect('/');
    });
});

app.post('/signup', upload.single('profilePhoto'), async (req, res) => {
    const { firstname, lastname, username, email, password } = req.body;
    const hashedPassword = await bcrypt.hash(password, 10);
    const userData = {
        firstname,
        lastname,
        username,
        email,
        password: hashedPassword,
        profilePicture: req.file ? `/uploads/${req.file.filename}` : null
    };

    const userPath = path.join(__dirname, 'data/users', `${username}.json`);
    try {
        await fs.writeJson(userPath, userData, { spaces: 2 });
        req.session.user = { username: username, firstName: firstname, lastName: lastname, profilePicture: userData.profilePicture };
        res.redirect('/dashboard');
    } catch (err) {
        console.error('Failed to save user data:', err);
        res.status(500).send({ error: err.message });
    }
});

app.post('/login', async (req, res) => {
    const { username, password } = req.body;
    const userPath = path.join(userDataPath, `${username}.json`);
    if (fs.existsSync(userPath)) {
        const userData = await fs.readJson(userPath);
        if (await bcrypt.compare(password, userData.password)) {
            req.session.user = {
                username: userData.username,
                firstName: userData.firstname,
                lastName: userData.lastname,
                profilePicture: userData.profilePicture
            };
            res.redirect('/dashboard');
        } else {
            res.status(401).send('Invalid username or password');
        }
    } else {
        res.status(404).send('User not found');
    }
});

app.post('/api/sensor_data', express.json(), (req, res) => {
    const { temperature, humidity, airQuality, gps, led } = req.body;
    const newEntry = { temperature, humidity, airQuality, gps, led, timestamp: new Date().toISOString() };

    try {
        let data = fs.existsSync(sensorDataFilePath) ? fs.readJsonSync(sensorDataFilePath) : [];
        data.push(newEntry);
        fs.writeJsonSync(sensorDataFilePath, data);
        io.emit('sensorData', newEntry);
        res.status(200).send('Data received and stored');
    } catch (err) {
        console.error('Error updating sensor data:', err);
        res.status(500).send('Internal Server Error: Could not update sensor data');
    }
});


app.post('/api/updateUser', upload.single('profilePhoto'), async (req, res) => {
    const { username } = req.session.user;
    const userPath = path.join(userDataPath, `${username}.json`);
    if (!fs.existsSync(userPath)) {
        return res.status(404).send('User not found');
    }

    const userData = await fs.readJson(userPath);
    const { firstname, lastname, password, file } = req.body;
    if (firstname) userData.firstname = firstname;
    if (lastname) userData.lastname = lastname;
    if (password && password.trim() !== '') {
        userData.password = await bcrypt.hash(password, 10);
    }
    if (req.file) {
        const oldFilePath = userData.profilePicture ? path.join(__dirname, 'uploads', userData.profilePicture) : null;
        if (oldFilePath && fs.existsSync(oldFilePath)) {
            await fs.unlink(oldFilePath);
        }
        userData.profilePicture = `/uploads/${req.file.filename}`;
    }

    try {
        await fs.writeJson(userPath, userData, { spaces: 2 });
        req.session.user = userData;
        res.send('User updated successfully');
    } catch (err) {
        console.error('Failed to save user data:', err);
        res.status(500).send({ error: err.message });
    }
});

// Middleware to check user authentication
function isAuthenticated(req, res, next) {
    if (!req.session.user) {
        res.status(401).send('Unauthorized');
    } else {
        next();
    }
}

// Error handling middleware
app.use((err, req, res, next) => {
    console.error('Internal Server Error:', err);
    res.status(500).send({ message: 'Internal Server Error', error: err.message });
});

// Server initialization
server.listen(PORT, () => console.log(`Server running on http://localhost:${PORT}`));

