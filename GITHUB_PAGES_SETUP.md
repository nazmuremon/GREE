# GitHub Pages Deployment Guide

Complete steps to deploy the Universal Gree Remote web UI to GitHub Pages.

## Prerequisites

- GitHub account (free tier works fine)
- Git installed locally

## Step 1: Create GitHub Repository

1. **Log in to GitHub**: https://github.com/login

2. **Create new repository**:
   - Click **+** icon → **New repository**
   - **Repository name**: `esp32-gree-remote-web`
   - **Description**: "Universal Gree Air Conditioner Remote Control via MQTT"
   - **Visibility**: Public (required for free GitHub Pages)
   - **Initialize**: Check "Add a README file"
   - Click **Create repository**

## Step 2: Clone Repository Locally

```bash
# Clone the repo
git clone https://github.com/YOUR_USERNAME/esp32-gree-remote-web.git
cd esp32-gree-remote-web

# Or if you prefer SSH:
git clone git@github.com:YOUR_USERNAME/esp32-gree-remote-web.git
```

Replace `YOUR_USERNAME` with your actual GitHub username.

## Step 3: Add Web UI Files

Copy the web interface files:

```bash
# From the ESP32 project directory:
cp -r github-pages-site/* esp32-gree-remote-web/

# Files to have:
# - index.html
# - README.md
# - .github/workflows/pages.yml
```

Or manually:
1. Copy `index.html` to repository root
2. Copy `README.md` to repository root
3. Create `.github/workflows/pages.yml` with deployment configuration

## Step 4: Configure GitHub Pages

1. **Go to repository Settings**:
   - Click **Settings** tab (near top-right)
   - Select **Pages** from left sidebar

2. **Build and deployment**:
   - Source: Select **Deploy from a branch**
   - Branch: Select **main** and **/ (root)**
   - Click **Save**

3. **GitHub Actions**:
   - Go to **Actions** tab
   - Confirm workflow is ready to deploy

## Step 5: Push to GitHub

```bash
# Stage all files
git add .

# Commit
git commit -m "Initial commit: Universal Gree Remote web UI"

# Push to GitHub
git push origin main
```

The workflow will automatically deploy to GitHub Pages.

## Step 6: Access Your Website

After ~1-2 minutes, your site will be live at:

```
https://YOUR_USERNAME.github.io/esp32-gree-remote-web/
```

Check deployment status:
- Go to **Actions** tab
- Click on the latest workflow run
- Verify it shows ✅ status

## Step 7: Update MQTT Configuration

If your MQTT broker details change, edit `index.html`:

```javascript
// Line ~85 in index.html
const MQTT_BROKER_HOST = 'broker.hivemq.com';
const MQTT_BROKER_PORT = 8001; // WebSocket port for browser
const MQTT_USERNAME = ''; // No authentication needed
const MQTT_PASSWORD = ''; // No authentication needed
```

Then push changes:
```bash
git add index.html
git commit -m "Update MQTT broker configuration"
git push origin main
```

## Step 8: Optional - Custom Domain

To use a custom domain (e.g., `remote.example.com`):

1. **Repository Settings → Pages**
2. **Custom domain**: Enter your domain
3. **Update DNS** records for your domain:
   - **A record** → `185.199.108.153`
   - **A record** → `185.199.109.153`
   - **A record** → `185.199.110.153`
   - **A record** → `185.199.111.153`
   - OR **CNAME** → `YOUR_USERNAME.github.io`
4. **Enforce HTTPS** (check the box)

## Troubleshooting

### "Cannot find index.html"

Check that `index.html` is in the repository root:
```bash
# From repo root, should show index.html
ls -la index.html
```

### GitHub Pages workflow failed

1. Go to **Actions** tab
2. Click failing workflow
3. Check error message
4. Common fixes:
   - File permissions: `chmod +x .`
   - Remove `.github/workflows` if using Branch deployment
   - Commit `_config.yml` if using Jekyll

### Website shows 404

1. Wait 2-3 minutes after push (GitHub Pages deployment takes time)
2. Check **Actions** tab - verify deployment shows ✅
3. Check repository Settings > Pages to confirm source

### MQTT not connecting

1. Check browser console (F12) for errors
2. Verify MQTT broker is reachable:
   ```bash
   ping broker.hivemq.com
   ```
3. Test MQTT directly:
   ```bash
   mosquitto_pub -h broker.hivemq.com \
     -t test -m "hello"
   ```

## Project URLs

After deployment, you'll have:

| URL | Purpose |
|-----|---------|
| `https://YOUR_USERNAME.github.io/esp32-gree-remote-web` | Main remote control UI |
| `https://github.com/YOUR_USERNAME/esp32-gree-remote-web` | Repository |
| `https://github.com/YOUR_USERNAME/esp32-gree-remote-web/actions` | Deployment status |

## Sharing with Others

Share the web UI URL:
- **Direct link**: https://YOUR_USERNAME.github.io/esp32-gree-remote-web
- **Mobile friendly**: Works on any device with a browser
- **No app installation**: Just open link
- **Real-time control**: Instant IR command transmission

## Example Repository Structure

After setup, your repo should look like:

```
esp32-gree-remote-web/
├── .github/
│   └── workflows/
│       └── pages.yml
├── index.html
├── README.md
├── LICENSE
└── .gitignore
```

## Next Steps

1. **Update Main Repository**: Link to this web UI from your main ESP32 repo
2. **Share Documentation**: Send link to GitHub Pages site
3. **Monitor Logs**: Check Actions tab for any deployment issues
4. **Backup**: Keep a local backup of your repository

## Support

For GitHub Pages issues:
- GitHub Docs: https://docs.github.com/en/pages
- GitHub Support: https://support.github.com

For MQTT issues:
- Paho.js Docs: https://www.eclipse.org/paho/clients/js/
- MQTT.org: https://mqtt.org

---

**Your web UI is now globally accessible from any device!** 🌍
