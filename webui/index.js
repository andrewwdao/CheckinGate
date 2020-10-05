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
// zip
var fs = require('fs');
const AdmZip = require('adm-zip');

var uploadImage = './public/images';
var directory = './public/exports/images/';
//Loads Variable evironment
dotenv.config({ path: './config/config.env' });

const PORT = process.env.PORT || 5000;
const HOST = process.env.HOST || 'localhost';
const USERNAME = process.env.USERNAMEDB || 'root';
const PASSWORD = process.env.PASSWORD || '';
const DATABASE = process.env.DATABASE || 'no_database';

app.set('view engine', 'ejs');
app.set('views', './views');
app.use(express.static(path.join(__dirname, 'public')));

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
    cb(null, 'public/uploads');
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
  res.render('exportDate', { HOST, PORT });
});

//export-date
app.post('/export-date', async (req, res) => {
  let StartDate = moment(req.body.txtStartDay + '00:00:00', 'DD/MM/YYYY HH:mm:ss').valueOf();
  let EndDate = moment(req.body.txtEndDay + '23:59:59', 'DD/MM/YYYY HH:mm:ss').valueOf();

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
        if (fileName[0] > StartDate && fileName[0] < EndDate) {
          fs.copyFile(__dirname + '/public/images/' + file, __dirname + '/public/exports/images/' + file, (err) => {
            if (err) return console.error(err);
            // console.log(file);
          });
        }
      });
    });

    //WHERE timestamp >= ${StartDate} AND timestamp <= ${EndDate}
    let sql = `SELECT * FROM checkin_events WHERE timestamp >= ${StartDate} AND timestamp <= ${EndDate}`;
    //console.log(sql);
    conn.query(sql, async function (err, result) {
      if (err) {
        console.log(err);
      } else {
        const jsonExport = JSON.parse(JSON.stringify(result));

        //for (let i = 0; i < result.length; i++) jsonExport[i]['image'] = 'images/' + jsonExport[i]['timestamp'] + '.jpg';

        //console.log(jsonExport);
        let workExcel = new excel.Workbook();
        let worksheet = workExcel.addWorksheet('check-in');

        //  WorkSheet Header
        worksheet.columns = [
          { header: 'gateId', key: 'gateId', width: 10 },
          { header: 'timestamp', key: 'timestamp', width: 20 },
          { header: 'datetime', key: 'datetime', width: 20 },
          { header: 'direction', key: 'direction', width: 20 },
          { header: 'rfidTag', key: 'rfidTag', width: 20 },
          { header: 'personalName', key: 'personalName', width: 20 },
          {
            header: 'personalCode',
            key: `personalCode`,
            width: 20,
          },
          {
            header: 'image',
            key: 'image',
            width: 30,
            outlineLevel: 1,
          },
        ];

        // Add Array Rows
        worksheet.addRows(jsonExport);
        // var i = 1;
        // let j = i + 1;
        // worksheet.spliceRows(1, 0, jsonExport);
        for (let i = 0; i < jsonExport.length; i++) {
          // note ***
          let cell = worksheet.getRow(i).getCell('image');
          cell.value = {
            text: 'images/' + jsonExport[i]['timestamp'] + '_pir1.jpg',
            hyperlink: 'images/' + jsonExport[i]['timestamp'] + '_pir1.jpg',
            tooltip: 'images/' + jsonExport[i]['timestamp'] + '_pir1.jpg',
          };
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

// home page
app.get('/', (req, res) => {
  conn.query('SELECT * FROM nhan_vien', function (err, result, fields) {
    if (err) throw err;
    res.render('home', { data: result, HOST, PORT });
  });
});

// edit user admin
app.get('/edit/:id', (req, res) => {
  var sql = `SELECT * FROM nhan_vien where id =${req.params.id}`;
  conn.query(sql, function (err, result) {
    if (err) throw err;
    res.render('editUser', { user: result, HOST, PORT });
  });
});

app.post('/edit', (req, res) => {
  var sql = `UPDATE nhan_vien SET ho_ten = "${req.body.txtHoten}", don_vi = "${req.body.txtDonvi}" , dien_thoai = "${req.body.txtDienthoai}"  WHERE id = ${req.body.userId}`;
  conn.query(sql, function (err, result) {
    if (err) throw err;
    console.log('1 record change');
    res.redirect('/');
  });
});

// delete user admin
app.get('/delete/:id', (req, res) => {
  var sql = `DELETE FROM nhan_vien WHERE id = ${req.params.id}`;
  conn.query(sql, function (err, result) {
    if (err) throw err;
    res.redirect('/');
  });
});

//delete all
app.get('/deleteall', (req, res) => {
  var sql = `DELETE FROM nhan_vien`;
  conn.query(sql, function (err, result) {
    if (err) throw err;
    res.redirect('/');
  });
});

//uploads file excel
app.get('/uploadexcel', (req, res) => {
  res.render('excel', { HOST, PORT });
});

app.post('/uploadexcel', (req, res) => {
  upload(req, res, function (err) {
    //console.log(req.file);
    if (err instanceof multer.MulterError) {
      console.log('Đã xảy ra lỗi Multer khi tải lên.');
    } else if (err) {
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
          //console.log(result);
          if (err) {
            //console.log(err);
            if (err.code === 'ER_DUP_ENTRY') console.log('duplicate');
          }
        });
      });
      res.redirect('/');
    }
  });
});

//Export Mysql to Excel
app.get('/export', (req, res) => {
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
});

app.listen(PORT, () => console.log(`App listening at http://${HOST}:${PORT}`.cyan.bold));
