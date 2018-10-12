const fs = require('fs');
const gulp = require('gulp');
const htmlmin = require('gulp-htmlmin');
const cleancss = require('gulp-clean-css');
const uglify = require('gulp-uglify');
const gzip = require('gulp-gzip');
const del = require('del');
const inline = require('gulp-inline');
const inlineImages = require('gulp-css-base64');
const favicon = require('gulp-base64-favicon');

const dataFolder = 'build/';

gulp.task('clean', function(done) {
    del([ dataFolder + '*']);
    done();
    return true;
});

gulp.task('buildfs_embeded', function(done) {

    var source = dataFolder + 'index.html.gz';
    var destination = dataFolder + 'index.html.gz.h';

    var wstream = fs.createWriteStream(destination);
    wstream.on('error', function (err) {
        console.log(err);
    });

    var data = fs.readFileSync(source);

    wstream.write('static const unsigned int index_html_gz_len = ' + data.length + ';\n');
    wstream.write('static const char index_html_gz[] PROGMEM = {\n    ');

    function toHex(x) {
        var prefix = '0x';
        if (x < 16) prefix = '0x0';
        return prefix + x.toString(16);
    }

    for (var i = 0; i < data.length; i++) {
        wstream.write(toHex(data[i]));
        if (i < data.length-1) {
            if (i % 12 >= 11) {
                wstream.write(',\n    ');
            } else {
                wstream.write(', ');
            }
        }
    }

    wstream.write('};\n');
    wstream.end();

    done();
});

gulp.task('buildfs_inline', function() {
    return gulp.src('src/*.html')
        // .pipe(favicon())
        .pipe(inline({
            base: 'src/',
            js: uglify,
            css: [cleancss, inlineImages],
            disabledTypes: ['svg', 'img']
        }))
        .pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        }))
        .pipe(gzip({ gzipOptions: { level: 9 } }))
        .pipe(gulp.dest(dataFolder));
});

gulp.task('default', gulp.series('clean', 'buildfs_inline', 'buildfs_embeded'));
