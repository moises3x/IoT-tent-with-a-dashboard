# Smart Tent Project

This project is a smart tent monitoring system built with Arduino and IoT, using a Node.js server with Express.js to handle user authentication and a dashboard for visualizing sensor data. The dashboard provides real-time monitoring of temperature, humidity, air quality, and weather conditions.

## Features

- **User Authentication**: Sign up and sign in functionality.
- **Real-time Data Visualization**: Displays temperature, humidity, air quality, and weather data in real-time.
- **Weather Integration**: Fetches current weather data using the MetaWeather API.
- **Interactive Dashboard**: Uses Chart.js to display data in a user-friendly manner.
- **Map Integration**: Displays the location of the tent using OpenStreetMap.

## Prerequisites

- Node.js
- npm (Node Package Manager)
- Arduino Uno with relevant sensors (Temperature, Humidity, Air Quality)
- MetaWeather API key

## Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/your-username/smart-tent.git
   cd smart-tent


2. **Install Dependencies**:
   ```bash
   npm install
   ```

3. **Setup Environment Variables**:
   Create a `.env` file in the root directory and add your MetaWeather API key.
   ```env
   METAWEATHER_API_KEY=your_api_key
   ```

4. **Run the Server**:
   ```bash
   node app.js
   ```

5. **Upload Arduino Sketch**:
   Upload your Arduino sketch to the Arduino Uno to start sending sensor data to the server.

## Usage

1. **Sign Up**:
   Visit `http://localhost:3000/signup` to create a new account.

2. **Sign In**:
   Visit `http://localhost:3000/signin` to log in with your account.

3. **Dashboard**:
   After signing in, you will be redirected to the dashboard at `http://localhost:3000/dashboard` where you can view real-time data visualizations and the current weather.

## File Structure

```
smart-tent/
│
├── views/
│   ├── dashboard.ejs
│   ├── signin.ejs
│   ├── signup.ejs
│
├── public/
│   ├── css/
│   │   └── styles.css
│   ├── js/
│   │   └── scripts.js
│
├── app.js
├── package.json
├── users.json
└── README.md
```

## API Endpoints

- **GET /signup**: Render the sign-up page.
- **POST /signup**: Handle sign-up form submission.
- **GET /signin**: Render the sign-in page.
- **POST /signin**: Handle sign-in form submission.
- **GET /dashboard**: Render the dashboard page.
- **GET /weather**: Fetch weather data from MetaWeather API.

## Technologies Used

- **Backend**: Node.js, Express.js
- **Frontend**: EJS, HTML, CSS, JavaScript, Chart.js
- **APIs**: MetaWeather API
- **Database**: JSON file for user data storage

## Contributing

1. Fork the repository.
2. Create a new branch (`git checkout -b feature/your-feature`).
3. Commit your changes (`git commit -am 'Add some feature'`).
4. Push to the branch (`git push origin feature/your-feature`).
5. Create a new Pull Request.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [MetaWeather API](https://www.metaweather.com/)
- [Chart.js](https://www.chartjs.org/)
- [OpenStreetMap](https://www.openstreetmap.org/)

```

Make sure to replace `your-username` with your actual GitHub username in the clone URL. Also, ensure that the `.env` setup matches how your application handles environment variables if you decide to use them.
