'use strict';

var nsfw = require('../src/');
var path = require('path');
var promisify = require('promisify-node');
var fse = promisify(require('fs-extra'));
var exec = promisify(function (command, options, callback) {
  return require('child_process').exec(command, options, callback);
});

jasmine.DEFAULT_TIMEOUT_INTERVAL = 240000;

var DEBOUNCE = 1000;
var TIMEOUT_PER_STEP = 3000;

describe('Node Sentinel File Watcher', function () {
  var workDir = path.resolve('./mockfs');

  beforeEach(function (done) {
    function makeDir(identifier) {
      return fse.mkdir(path.join(workDir, 'test' + identifier)).then(function () {
        return fse.mkdir(path.join(workDir, 'test' + identifier, 'folder' + identifier));
      }).then(function () {
        return fse.open(path.join(workDir, 'test' + identifier, 'testing' + identifier + '.file'), 'w');
      }).then(function (fd) {
        return fse.write(fd, 'testing').then(function () {
          return fd;
        });
      }).then(function (fd) {
        return fse.close(fd);
      });
    }
    // create a file System
    return fse.stat(workDir).then(function () {
      return fse.remove(workDir);
    }, function () {}).then(function () {
      return fse.mkdir(workDir);
    }).then(function () {
      var promises = [];
      for (var i = 0; i < 5; ++i) {
        promises.push(makeDir(i));
      }
      return Promise.all(promises);
    }).then(done);
  });

  afterEach(function (done) {
    return fse.remove(workDir).then(done);
  });

  describe('Basic', function () {
    it('can watch a single file', function (done) {
      var file = 'testing1.file';
      var inPath = path.resolve(workDir, 'test1');
      var filePath = path.join(inPath, file);
      var changeEvents = 0;
      var createEvents = 0;
      var deleteEvents = 0;

      function findEvents(element) {
        if (element.action === nsfw.actions.MODIFIED && element.directory === path.resolve(inPath) && element.file === file) {
          changeEvents++;
        } else if (element.action === nsfw.actions.CREATED && element.directory === path.resolve(inPath) && element.file === file) {
          createEvents++;
        } else if (element.action === nsfw.actions.DELETED && element.directory === path.resolve(inPath) && element.file === file) {
          deleteEvents++;
        }
      }

      var watch = void 0;

      return nsfw(filePath, function (events) {
        return events.forEach(findEvents);
      }, { debounceMS: 100 }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        var fd = fse.openSync(filePath, 'w');
        fse.writeSync(fd, 'Bean bag video games at noon.');
        return fse.close(fd);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return fse.remove(filePath);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        var fd = fse.openSync(filePath, 'w');
        fse.writeSync(fd, 'His watch has ended.');
        return fse.close(fd);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return watch.stop();
      }).then(function () {
        expect(changeEvents).toBeGreaterThan(0);
        expect(createEvents).toBe(1);
        expect(deleteEvents).toBe(1);
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });

    it('can listen for a create event', function (done) {
      var file = 'another_test.file';
      var inPath = path.resolve(workDir, 'test2', 'folder2');
      var eventFound = false;

      function findEvent(element) {
        if (element.action === nsfw.actions.CREATED && element.directory === path.resolve(inPath) && element.file === file) {
          eventFound = true;
        }
      }

      var watch = void 0;

      return nsfw(workDir, function (events) {
        return events.forEach(findEvent);
      }, { debounceMS: DEBOUNCE }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return fse.open(path.join(inPath, file), 'w');
      }).then(function (fd) {
        return fse.write(fd, 'Peanuts, on occasion, rain from the skies.').then(function () {
          return fd;
        });
      }).then(function (fd) {
        return fse.close(fd);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        expect(eventFound).toBe(true);
        return watch.stop();
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });

    it('can listen for a delete event', function (done) {
      var file = 'testing3.file';
      var inPath = path.resolve(workDir, 'test3');
      var eventFound = false;

      function findEvent(element) {
        if (element.action === nsfw.actions.DELETED && element.directory === path.resolve(inPath) && element.file === file) {
          eventFound = true;
        }
      }

      var watch = void 0;

      return nsfw(workDir, function (events) {
        return events.forEach(findEvent);
      }, { debounceMS: DEBOUNCE }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return fse.remove(path.join(inPath, file));
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        expect(eventFound).toBe(true);
        return watch.stop();
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });

    it('can listen for a modify event', function (done) {
      var file = 'testing0.file';
      var inPath = path.resolve(workDir, 'test0');
      var eventFound = false;

      function findEvent(element) {
        if (element.action === nsfw.actions.MODIFIED && element.directory === path.resolve(inPath) && element.file === file) {
          eventFound = true;
        }
      }

      var watch = void 0;

      return nsfw(workDir, function (events) {
        return events.forEach(findEvent);
      }, { debounceMS: DEBOUNCE }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return fse.open(path.join(inPath, file), 'w');
      }).then(function (fd) {
        return fse.write(fd, 'At times, sunflower seeds are all that is life.').then(function () {
          return fd;
        });
      }).then(function (fd) {
        return fse.close(fd);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        expect(eventFound).toBe(true);
        return watch.stop();
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });

    it('can run multiple watchers at once', function (done) {
      var dirA = path.resolve(workDir, 'test0');
      var fileA = 'testing1.file';
      var dirB = path.resolve(workDir, 'test1');
      var fileB = 'testing0.file';
      var events = 0;

      function findEvent(element) {
        if (element.action === nsfw.actions.CREATED) {
          if (element.directory === dirA && element.file === fileA) {
            events++;
          } else if (element.directory === dirB && element.file === fileB) {
            events++;
          }
        }
      }

      var watchA = void 0;
      var watchB = void 0;

      return nsfw(dirA, function (events) {
        return events.forEach(findEvent);
      }, { debounceMS: DEBOUNCE }).then(function (_w) {
        watchA = _w;
        return watchA.start();
      }).then(function () {
        return nsfw(dirB, function (events) {
          return events.forEach(findEvent);
        }, { debounceMS: DEBOUNCE }).then(function (_w) {
          watchB = _w;
          return watchB.start();
        });
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return fse.open(path.join(dirA, fileA), 'w');
      }).then(function (fd) {
        return new Promise(function (resolve) {
          setTimeout(function () {
            return resolve(fd);
          }, TIMEOUT_PER_STEP);
        });
      }).then(function (fd) {
        return fse.write(fd, 'At times, sunflower seeds are all that is life.').then(function () {
          return fd;
        });
      }).then(function (fd) {
        return fse.close(fd);
      }).then(function () {
        return fse.open(path.join(dirB, fileB), 'w');
      }).then(function (fd) {
        return new Promise(function (resolve) {
          setTimeout(function () {
            return resolve(fd);
          }, TIMEOUT_PER_STEP);
        });
      }).then(function (fd) {
        return fse.write(fd, 'At times, sunflower seeds are all that is life.').then(function () {
          return fd;
        });
      }).then(function (fd) {
        return fse.close(fd);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        expect(events).toBe(2);
        return watchA.stop();
      }).then(function () {
        return watchB.stop();
      }).then(done, function () {
        return watchA.stop().then(function () {
          return watchB.stop();
        }).then(function (err) {
          return done.fail(err);
        });
      });
    });
  });

  describe('Recursive', function () {
    it('can listen for the creation of a deeply nested file', function (done) {
      var paths = ['d', 'e', 'e', 'p', 'f', 'o', 'l', 'd', 'e', 'r'];
      var file = 'a_file.txt';
      var foundFileCreateEvent = false;

      function findEvent(element) {
        if (element.action === nsfw.actions.CREATED && element.directory === path.join.apply(path, [workDir].concat(paths)) && element.file === file) {
          foundFileCreateEvent = true;
        }
      }

      var watch = void 0;

      var directory = workDir;

      return nsfw(workDir, function (events) {
        return events.forEach(findEvent);
      }, { debounceMS: DEBOUNCE }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return paths.reduce(function (chain, dir) {
          directory = path.join(directory, dir);
          var nextDirectory = directory;
          return chain.then(function () {
            return fse.mkdir(nextDirectory);
          });
        }, Promise.resolve());
      }).then(function () {
        return fse.open(path.join(directory, file), 'w');
      }).then(function (fd) {
        return fse.close(fd);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        expect(foundFileCreateEvent).toBe(true);
        return watch.stop();
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });

    it('can listen for the destruction of a directory and its subtree', function (done) {
      var inPath = path.resolve(workDir, 'test4');
      var deletionCount = 0;

      function findEvent(element) {
        if (element.action === nsfw.actions.DELETED) {
          if (element.directory === path.resolve(inPath) && (element.file === 'testing4.file' || element.file === 'folder4')) {
            deletionCount++;
          } else if (element.directory === workDir && element.file === 'test4') {
            deletionCount++;
          }
        }
      }

      var watch = void 0;

      return nsfw(workDir, function (events) {
        return events.forEach(findEvent);
      }, { debounceMS: DEBOUNCE }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return fse.remove(inPath);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        expect(deletionCount).toBeGreaterThan(2);
        return watch.stop();
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });
  });

  describe('Errors', function () {
    it('can gracefully recover when the watch folder is deleted', function (done) {
      var inPath = path.join(workDir, 'test4');
      var erroredOut = false;
      var watch = void 0;

      return nsfw(inPath, function () {}, { debounceMS: DEBOUNCE, errorCallback: function errorCallback() {
          erroredOut = true;
        }
      }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return fse.remove(inPath);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        expect(erroredOut).toBe(true);
        return watch.stop();
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });
  });

  describe('Stress', function () {
    var stressRepoPath = path.resolve('nsfw-stress-test');

    beforeEach(function (done) {
      return exec('git clone https://github.com/implausible/nsfw-stress-test').then(done);
    });

    it('does not segfault under stress', function (done) {
      var count = 0;
      var watch = void 0;
      var errorRestart = Promise.resolve();

      return nsfw(stressRepoPath, function () {
        count++;
      }, {
        errorCallback: function errorCallback() {
          errorRestart = watch.stop().then(watch.start);
        }
      }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return fse.remove(path.join(stressRepoPath, 'folder'));
      }).then(function () {
        return errorRestart;
      }).then(function () {
        expect(count).toBeGreaterThan(0);
        return watch.stop();
      }).then(function () {
        return fse.remove(stressRepoPath);
      }).then(function () {
        return fse.mkdir(stressRepoPath);
      }).then(function () {
        return nsfw(stressRepoPath, function () {
          count++;
        }, {
          errorCallback: function errorCallback() {
            errorRestart = watch.stop().then(watch.start);
          }
        });
      }).then(function (_w) {
        watch = _w;
        count = 0;
        errorRestart = Promise.resolve();
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          return setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return exec('git clone https://github.com/implausible/nsfw-stress-test ' + path.join('nsfw-stress-test', 'test'));
      }).then(function () {
        return fse.stat(path.join(stressRepoPath, 'test'));
      }).then(function () {
        return fse.remove(path.join(stressRepoPath, 'test'));
      }).then(function () {
        return errorRestart;
      }).then(function () {
        expect(count).toBeGreaterThan(0);
        count = 0;
        return watch.stop();
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });

    it('creates and destroys many watchers', function (done) {
      var watcher = null;
      var promiseChain = Promise.resolve();

      for (var i = 0; i < 50; i++) {
        promiseChain = promiseChain.then(function () {
          return nsfw(stressRepoPath, function () {});
        }).then(function (w) {
          watcher = w;
          return watcher.start();
        }).then(function () {
          return watcher.stop();
        });
      }

      promiseChain.then(done, function (err) {
        return done.fail(err);
      });
    });

    afterEach(function (done) {
      return fse.remove(stressRepoPath).then(done);
    });
  });

  describe('Unicode support', function () {
    var watchPath = path.join(workDir, 'は');
    beforeEach(function (done) {
      return fse.mkdir(watchPath).then(done);
    });

    it('supports watching unicode directories', function (done) {
      var file = 'unicoded_right_in_the.talker';
      var eventFound = false;

      function findEvent(element) {
        if (element.action === nsfw.actions.CREATED && element.directory === watchPath && element.file === file) {
          eventFound = true;
        }
      }

      var watch = void 0;

      return nsfw(workDir, function (events) {
        return events.forEach(findEvent);
      }, { debounceMS: DEBOUNCE }).then(function (_w) {
        watch = _w;
        return watch.start();
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        return fse.open(path.join(watchPath, file), 'w');
      }).then(function (fd) {
        return fse.write(fd, 'Unicode though.').then(function () {
          return fd;
        });
      }).then(function (fd) {
        return fse.close(fd);
      }).then(function () {
        return new Promise(function (resolve) {
          setTimeout(resolve, TIMEOUT_PER_STEP);
        });
      }).then(function () {
        expect(eventFound).toBe(true);
        return watch.stop();
      }).then(done, function () {
        return watch.stop().then(function (err) {
          return done.fail(err);
        });
      });
    });
  });
});