# Metro Configuration for Expo

This is the default configuration for Metro, the JavaScript bundler for React Native.

## Default Configuration

```javascript
const { getDefaultConfig } = require('expo/metro-config');

const config = getDefaultConfig(__dirname);

module.exports = config;
```

For more information about Metro configuration, see:
- https://facebook.github.io/metro/docs/getting-started
- https://docs.expo.dev/versions/latest/guides/customizing-metro
