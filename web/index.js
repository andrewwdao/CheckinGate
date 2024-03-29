const express = require('express');
const app = express();
const colors = require('colors');
const dotenv = require('dotenv');
const mysql = require('mysql');
const multer = require('multer');
const path = require('path');
const readXlsxFile = require('read-excel-file/node');
const excel = require('exceljs');
const moment = require('moment');
const session = require('express-session');
// zip
var fs = require('fs');
const AdmZip = require('adm-zip');

var uploadImage = __dirname + '/public/images';
var directory = __dirname + '/public/exports/images/';
//Loads Variable evironment
dotenv.config({ path: __dirname + '/config/config.env' });

const PORT = process.env.PORT || 5000;
const HOST = process.env.HOST || 'localhost';
const USERNAME = process.env.USERNAMEDB || 'root';
const PASSWORD = process.env.PASSWORD || '';
const DATABASE = process.env.DATABASE || 'no_database';

app.set('view engine', 'ejs');
app.set('views', __dirname + '/views');
app.use(express.static(__dirname + '/public'));
app.use(
  session({
    secret: 'checkingate admin',
    resave: true,
    saveUninitialized: true,
  })
);
// middleware to make 'user' available to all templates
app.use(function (req, res, next) {
  res.locals.loggedHome = req.session.loggedHome;
  next();
});

global.__basedir = __dirname;

//body-parser
var bodyParser = require('body-parser');
app.use(bodyParser.urlencoded({ extended: false }));

// Mysql
const conn = mysql.createConnection({
  host: 'localhost',
  user: USERNAME,
  password: PASSWORD,
  database: DATABASE,
  dateStrings: true,
});
conn.connect(function (err) {
  //  throw err;
  if (err) {
    console.log('Connect fail ....'.red.underline);
  } else {
    console.log('Connected database succesfully'.yellow.underline);
  }
});

//multer
var storage = multer.diskStorage({
  destination: function (req, file, cb) {
    cb(null, __dirname + '/public/uploads');
  },
  filename: function (req, file, cb) {
    cb(null, Date.now() + '-' + file.originalname);
  },
});
var upload = multer({
  storage: storage,
  fileFilter: function (req, file, cb) {
    // console.log(file);
    if (file.mimetype.includes('excel') || file.mimetype.includes('spreadsheetml')) {
      cb(null, true);
    } else {
      return cb(new Error('Only excel are allowed!'));
    }
  },
}).single('fileExcel');

//export-date
app.get('/export-date', (req, res) => {
  if(req.session.loggedHome){
    res.render('exportDate', { HOST, PORT });
  }else{
    res.redirect('/');
  }
});

//export-date
app.post('/export-date', async (req, res) => {
  let StartDate = moment(req.body.txtStartDay + ' 00:00:00', 'DD/MM/YYYY HH:mm:ss').valueOf();
  let EndDate = moment(req.body.txtEndDay + ' 23:59:59', 'DD/MM/YYYY HH:mm:ss').valueOf();
  console.log(`${StartDate}  ${EndDate}`);
  if (StartDate > EndDate) {
    res.send('Ngày bắt đầu phải bé hơn ngày kết thúc');
  } else {
    fs.readdir(directory, async (err, files) => {
      if (err) {
        console.log('MSG' + err);
      } else {
        for (const file of files) {
          fs.unlink(path.join(directory, file), (err) => {
            if (err) throw err;
          });
        }
      }
      await fs.readdirSync(uploadImage).forEach((file) => {
        var fileName = file.split('_');
        //console.log(file);
        if (fileName[0] >= StartDate && fileName[0] <= EndDate) {
          fs.copyFile(__dirname + '/public/images/' + file, __dirname + '/public/exports/images/' + file, async (err) => {
            if (err) return console.error(err);
            console.log(file);
          });
        }
      });
    });

    //WHERE timestamp >= ${StartDate} AND timestamp <= ${EndDate}
    let sql = `SELECT gateId, datetime, direction, rfidTag, personalName, personalCode, data FROM checkin_events as c, events as e WHERE e.timestamp >= ${StartDate} AND e.timestamp <= ${EndDate} AND e.timestamp = c.timestamp`;
    //console.log(sql);
    conn.query(sql, async function (err, result) {
      if (err) {
        console.log(err);
      } else {
        const jsonExport = JSON.parse(JSON.stringify(result));
        let workExcel = new excel.Workbook();
        let worksheet = workExcel.addWorksheet('check-in');

        //  WorkSheet Header
        worksheet.columns = [
          { header: 'Gate ID', key: 'gateId', width: 10 },
          //{ header: 'timestamp', key: 'timestamp', width: 20 },
          { header: 'Date time', key: 'datetime', width: 20 },
          { header: 'Direction', key: 'direction', width: 20 },
          { header: 'RFID Tag', key: 'rfidTag', width: 20 },
          { header: 'Personal name', key: 'personalName', width: 20 },
          { header: 'Personal code', key: `personalCode`, width: 20 },
          {
            header: 'Photo 1.1',
            key: 'data',
            width: 10,
          },
          {
            header: 'Photo 1.2',
            key: 'data2',
            width: 10,
          },
          {
            header: 'Photo 2.1',
            key: 'data3',
            width: 10,
          },
          {
            header: 'Photo 2.2',
            key: 'data4',
            width: 10,
          },
        ];

        // Add Array Rows
        worksheet.addRows(jsonExport);
        for (let i = 2; i <= jsonExport.length + 1; i++) {
          let dataCol = worksheet.getRow(i).getCell('data');
          let checkinTime = dataCol.value.split(',')[1].split(':')[1];
          let enterTime = dataCol.value.split(',')[3].split(':')[1];

          let pir = worksheet.getRow(i).getCell('direction') == 'ra' ? 'pir1' : 'pir2';

          let imageCell = worksheet.getRow(i).getCell('data');
          imageCell.value = {
            text: 'View image',
            hyperlink: 'images/' + enterTime + '_pir1_1.jpg',
            tooltip: 'images/' + enterTime + '_pir1_1.jpg',
          };
          imageCell.font = { color: { argb: '000004db' } };

          imageCell = worksheet.getRow(i).getCell('data2');
          imageCell.value = {
            text: 'View image',
            hyperlink: 'images/' + enterTime + '_pir2_1.jpg',
            tooltip: 'images/' + enterTime + '_pir2_1.jpg',
          };
          imageCell.font = { color: { argb: '000004db' } };

          imageCell = worksheet.getRow(i).getCell('data3');
          imageCell.value = {
            text: 'View image',
            hyperlink: 'images/' + checkinTime + '_pir1_1.jpg',
            tooltip: 'images/' + checkinTime + '_pir1_1.jpg',
          };
          imageCell.font = { color: { argb: '000004db' } };

          imageCell = worksheet.getRow(i).getCell('data4');
          imageCell.value = {
            text: 'View image',
            hyperlink: 'images/' + checkinTime + '_pir2_1.jpg',
            tooltip: 'images/' + checkinTime + '_pir2_1.jpg',
          };
          imageCell.font = { color: { argb: '000004db' } };
        }

        // Write to File
        const EE = await workExcel.xlsx.writeFile(__dirname + '/public/exports/' + 'check-in.xlsx').then(function () {
          console.log('file saved');
        });

        const zip = new AdmZip();
        const downloadName = `checkin_${Date.now()}`;
        const uploadDir = fs.readdirSync(__dirname + '/public/exports/images/');
        console.log(uploadDir.length);
        for (var i = 0; i < uploadDir.length; i++) {
          await zip.addLocalFile(__dirname + '/public/exports/images/' + uploadDir[i], `${downloadName}/images`);
        }
        await zip.addLocalFile(__dirname + '/public/exports/' + 'check-in.xlsx', `${downloadName}`);

        // Define zip file name
        const data = zip.toBuffer();

        // code to download zip file
        res.set('Content-Type', 'application/octet-stream');
        res.set('Content-Disposition', `attachment; filename=${downloadName}.zip`);
        res.set('Content-Length', data.length);
        res.send(data);
      }
    });
  }
});

//login
app.get('/login', (req, res) => {
  res.render('auth/login', { HOST, PORT });
});

app.post('/authLogin', (req, res) => {
  const username = req.body.txtUsername;
  const password = req.body.txtPassword;
  const sql = `SELECT * FROM admin WHERE username='${username}' AND password='${password}'`;
  if (username && password) {
    conn.query(sql, (err, result) => {
      // console.log(result[0].fullname);
      if (err) return res.send(err.message);
      if (result.length > 0) {
        req.session.loggedHome = true;
        req.session.fullname = result[0].fullname;
        return res.redirect('/');
      } else {
        res.render('auth/login', { msg: 'Invalid username or password' });
      }
      // return res.end();
    });
  } else {
    res.render('auth/login', { msg: 'Please enter username and password' });
    // return res.send('Please enter username and password');
  }
});
//logout
app.get('/logout', (req, res) => {
  req.session.loggedHome = false;
  return res.redirect('/');
});

// home page
app.get('/', (req, res) => {
  if (req.session.loggedHome) {
  conn.query('SELECT * FROM nhan_vien', function (err, result, fields) {
    if (err) throw err;
    const getFullname = req.session.fullname;
    res.render('home', { data: result, HOST, PORT ,getFullname});
  });
  } else {
    res.render('auth/login', { HOST, PORT });
  }
});

// edit user admin
app.get('/edit/:id', (req, res) => {
  if (req.session.loggedHome) {
    var sql = `SELECT * FROM nhan_vien where id =${req.params.id}`;
    conn.query(sql, function (err, result) {
      if (err) throw err;
      
      res.render('editUser', { user: result, HOST, PORT });
    });
  }else{
    res.redirect('/');
  }

});

app.post('/edit', (req, res) => {
  var sql = `UPDATE nhan_vien SET rfid_tag = "${req.body.txtRfidTag}", ho_ten = "${req.body.txtHoten}", don_vi = "${req.body.txtDonvi}" , dien_thoai = "${req.body.txtDienthoai}"  WHERE id = ${req.body.userId}`;
  conn.query(sql, function (err, result) {
    if (err) throw err;
    console.log('1 record change');
    res.redirect('/');
  });
});

// delete user admin
app.get('/delete/:id', (req, res) => {
  if (req.session.loggedHome) {
  var sql = `DELETE FROM nhan_vien WHERE id = ${req.params.id}`;
  conn.query(sql, function (err, result) {
    if (err) throw err;
    res.redirect('/');
  });
}else{
  res.redirect('/');
}
});

//delete all
app.get('/deleteall', (req, res) => {
  if (req.session.loggedHome) {
  var sql = `DELETE FROM nhan_vien`;
  conn.query(sql, function (err, result) {
    if (err) throw err;
    res.redirect('/');
  });
}else{
  res.redirect('/');
}
});

//uploads file excel
app.get('/uploadexcel', (req, res) => {
  if (req.session.loggedHome) {
  res.render('excel', { HOST, PORT });
  }else{
    res.redirect('/');
  }
});

app.post('/uploadexcel', (req, res) => {
  if (req.session.loggedHome) {
  upload(req, res, function (err) {
    // console.log(req.file);
    if (err instanceof multer.MulterError) {
      console.log('Đã xảy ra lỗi Multer khi tải lên.');
    } else if (err) {
      console.log(err);
      res.render('alert', { errMess: 'Chỉ được phép tải lên file excel ' });
      //console.log("An unknown error occurred when uploading." + err);
    } else if (!req.file) {
      res.render('alert', {
        errMess: 'Vui lòng chọn file trước khi upload ',
      });
    } else {
      //console.log(req.file);
      // (req.file.filename)
      let filePath = __basedir + '/public/uploads/' + req.file.filename;

      readXlsxFile(filePath).then((rows) => {
        //console.log(rows);
        // Remove Header ROW
        rows.shift();
        var sql = `INSERT IGNORE  INTO nhan_vien ( manv, rfid_tag, ho_ten ,don_vi ,dien_thoai ,email) VALUES ?`;

        conn.query(sql, [rows], function (err, result) {
          // console.log(result);
          console.log(rows);
          if (err) {
            //console.log(err);
            if (err.code === 'ER_DUP_ENTRY') console.log('duplicate');
          }
        });
      });
      res.redirect('/');
    }
  });
}else{
  res.redirect('/');
}
});

//Export Mysql to Excel
app.get('/export', (req, res) => {
  if (req.session.loggedHome) {
  conn.query('SELECT * FROM nhan_vien', function (err, result, fields) {
    if (err) throw err;
    const jsonExport = JSON.parse(JSON.stringify(result));
    let workExcel = new excel.Workbook();
    let worksheet = workExcel.addWorksheet('Export');

    //  WorkSheet Header
    worksheet.columns = [
      { header: 'manv', key: 'manv', width: 20 },
      { header: 'rfid_tag', key: 'rfid_tag', width: 20 },
      { header: 'ho_ten', key: 'ho_ten', width: 20 },
      { header: 'don_vi', key: 'don_vi', width: 20 },
      { header: 'dien_thoai', key: 'dien_thoai', width: 20 },
      {
        header: 'Email',
        key: 'email',
        width: 20,
        outlineLevel: 1,
      },
    ];

    // Add Array Rows
    worksheet.addRows(jsonExport);

    res.setHeader('Content-Type', 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet');
    res.setHeader('Content-Disposition', 'attachment; filename=' + 'nhan_vien.xlsx');

    // Write to File
    workExcel.xlsx
      .write(res)
      .then(function () {
        console.log('file saved!');
      })
      .catch(function (err) {
        console.log(err);
      });
  });
}else{
  res.redirect('/');
}
});

app.listen(PORT, () => console.log(`App listening at http://${HOST}:${PORT}`.cyan.bold));
