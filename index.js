
/**
 * Module dependencies.
 */

var request = require('superagent')
  , sprintf = require('printf')
  , bytes = require('bytes')
  , path = require('path')
  , fs = require('fs')

/**
 * printf helper.
 */

function printf() {
  console.log(sprintf.apply(null, arguments));
}

/**
 * Log helper.
 */

function log(type, msg, color) {
  color = color || 36;
  printf('  \033[' + color + 'm%10s\033[0m : \033[90m%s\033[m', type, msg);
}

/**
 * Error helper.
 */

function error(name, res) {
  log('error', 'got ' + res.status + ' response', 31);
  process.exit(1);
}

/**
 * Install `name`.
 *
 * @param {String} name
 * @param {Object} options
 * @api private
 */

exports.install = function(name, options){
  var url = 'https://raw.github.com/' + name + '/master/package.json';

  log('install', name);
  request
  .get(url)
  .end(function(res){
    if (res.error) return error(name, res);
    var obj = JSON.parse(res.text);
    if (!obj.src) return log('error', '.src missing', 31);
    obj.src.forEach(function(file){
      fetch(name, file, options);
    });
  });
};

/**
 * Fetch `name`'s `file` and write to ./src.
 *
 * @param {String} name
 * @param {String} file
 * @param {Object} options
 * @api private
 */

function fetch(name, file, options) {
  var url = 'https://raw.github.com/' + name + '/master/' + file;
  var dst = options.to + '/' + path.basename(file);

  log('fetch', file);
  request
  .get(url)
  .end(function(res){
    if (res.error) return error(name, res);
    fs.writeFile(dst, res.text, function(err){
      if (err) throw err;
      log('write', dst + ' - ' + bytes(res.text.length));
    });
  });
}
