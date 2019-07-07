module.exports = {
  webpack: function(config, env) {
    if (env === "production") {
      config.output.filename = '[name].js';
      config.output.chunkFilename = '[name].chunk.js';
      config.plugins[5].options.chunkFilename = '[name].css';
    }

    return config;
  }
};
