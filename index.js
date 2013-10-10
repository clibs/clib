
/**
 * Module dependencies.
 */

var exec = require('child_process').exec;
var request = require('superagent');
var wiki = require('wiki-registry');
var command = require('shelly');
var sprintf = require('printf');
var bytes = require('bytes');
var path = require('path');
var fs = require('fs');

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

function error(name, res, url) {
  log('error', res.status + ' response (' + url + ')', 31);
  process.exit(1);
}

/**
 * Search via `query`.
 *
 * TODO: local cache or server like component
 * TODO: disply categories
 *
 * @param {String} query
 * @param {Function} fn
 * @api public
 */

exports.search = function(query, fn){
  var url = 'https://github.com/clibs/clib/wiki/Packages';
  wiki(url, function(err, cats){
    if (err) return fn(err);

    var pkgs = [];

    for (var cat in cats) {
      pkgs = pkgs.concat(cats[cat].filter(matches(query)));
    }

    fn(null, pkgs);
  });
};

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
    if (res.error) return error(name, res, url);
    var conf = JSON.parse(res.text);

    // bins
    if (conf.install) executable(conf);

    // scripts
    if (conf.src) {
      conf.src.forEach(function(file){
        fetch(name, file, options);
      });
    }
  });
};

/**
 * Fetch tarball, unpack, and install.
 *
 * @param {Object} conf
 * @api public
 */

function executable(conf) {
  var url = 'https://github.com/' + conf.repo + '/archive/' + conf.version + '.tar.gz';
  var name = conf.repo.replace(/\//g, '-');
  var tarball = fs.createWriteStream('/tmp/' + name);

  log('fetch', url);
  request
  .get(url)
  .buffer(false)
  .end(function(res){
    if (res.error) return error(conf.repo, res, url);
    res.pipe(tarball);
    res.on('end', function(){
      log('unpack', tarball.path);
      var cmd = command('cd /tmp && tar -zxf ?', name);
      exec(cmd, function(err){
        if (err) return log('error', err.message, 31);
        var cmd = command('cd /tmp/?-? && ' + conf.install, conf.name, conf.version);
        log('exec', conf.install);
        exec(cmd, function(err){
          if (err) return log('error', err.message, 31);
        });
      });
    });
  });
}

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
    if (res.error) return error(name, res, url);
    fs.writeFile(dst, res.text, function(err){
      if (err) throw err;
      log('write', dst + ' - ' + bytes(res.text.length));
    });
  });
}

/**
 * Package filter based on `query`.
 *
 * @param {String} query
 * @return {Function}
 * @api private
 */

function matches(query) {
  var words = query.split(/\s+/g);
  return function(pkg){
    return words.every(function(word){
      word = word.toLowerCase();
      return ~pkg.description.toLowerCase().indexOf(word)
        || ~pkg.name.indexOf(word);
    });
  }
}
