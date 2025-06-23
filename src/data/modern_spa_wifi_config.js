/*
===============================
SPA WIFI CONFIG - VERSIONE PULITA
===============================
Usa l'IP corrente dalla barra degli indirizzi
*/

/*
===============================
SPA WIFI CONFIG - VERSIONE PULITA
===============================
Usa l'IP corrente dalla barra degli indirizzi
*/

class WiFiConfigAPI {
    constructor() {
        // Usa l'IP corrente del browser - semplice e funziona sempre
        this.baseUrl = `${window.location.protocol}//${window.location.host}`;
        console.log(`🌐 API Base URL: ${this.baseUrl}`);
    }

    async fetchJSON(endpoint, options = {}) {
        try {
            const config = {
                headers: {
                    'Content-Type': 'application/json',
                    ...options.headers
                },
                ...options
            };

            const url = `${this.baseUrl}${endpoint}`;
            console.log(`🌐 Fetching: ${url}`);
            
            const response = await fetch(url, config);
            
            if (!response.ok) {
                const errorData = await response.json().catch(() => ({}));
                throw new Error(errorData.message || `HTTP ${response.status}: ${response.statusText}`);
            }

            return await response.json();
        } catch (error) {
            console.error(`API Error [${endpoint}]:`, error);
            throw error;
        }
    }

    // ===============================
    // API WIFI CONFIG
    // ===============================
    
    async loadConfig() {
        try {
            console.log('📥 Loading WiFi configuration...');
            const config = await this.fetchJSON('/api/wifi/config');
            console.log('Raw API response:', config);
            
            // Estrai info reti
            const networks = config.configured_networks || [];
            const currentNetwork = networks.find(n => n.is_current);
            const backupNetwork = networks.find(n => !n.is_current);
            
            console.log('✅ Configuration loaded:', {
                primary: currentNetwork?.ssid || 'None',
                backup: backupNetwork?.ssid || 'None',
                connected: config.connected || false,
                networks_count: networks.length
            });
            
            return {
                success: true,
                // Dati originali API
                ...config,
                // Campi per form UI
                primarySSID: currentNetwork?.ssid || '',
                primaryPassword: '',
                backupSSID: backupNetwork?.ssid || '',
                backupPassword: '',
                autoConnect: networks.length > 0,
                // Oggetti per UI
                system: {
                    uptime: config.uptime || 0,
                    heap_free: config.heap_free || 0,
                    heap_total: 320000,
                    cpu_freq: 240
                },
                wifi: {
                    status: config.status || 'disconnected',
                    ssid: config.current_ssid || '',
                    rssi: config.rssi || -100,
                    ip: config.ip || '',
                    quality: this.calculateSignalQuality(config.rssi || -100)
                }
            };
        } catch (error) {
            console.error('❌ Config load failed:', error);
            return {
                success: false,
                error: error.message,
                primarySSID: '',
                primaryPassword: '',
                backupSSID: '',
                backupPassword: '',
                connected: false,
                configured_networks: []
            };
        }
    }

    async saveConfig(configData) {
        try {
            console.log('💾 Saving WiFi configuration...', {
                primary: configData.primarySSID,
                backup: configData.backupSSID || 'None',
                primaryPassword: configData.primaryPassword ? '[PROVIDED]' : '[EMPTY]',
                backupPassword: configData.backupPassword ? '[PROVIDED]' : '[EMPTY]'
            });

            this.validateConfig(configData);

            // Carica la configurazione corrente
            const currentConfig = await this.loadConfig();
            
            // Verifica se ci sono modifiche sostanziali
            const hasSSIDChanges = !currentConfig.success || 
                                  currentConfig.primarySSID !== configData.primarySSID ||
                                  currentConfig.backupSSID !== configData.backupSSID;
            
            const hasPasswordChanges = configData.primaryPassword.trim() !== '' || 
                                      configData.backupPassword.trim() !== '' ||
                                      configData.primaryOpenNetwork ||
                                      configData.backupOpenNetwork;
            
            const hasAutoConnectChanges = !currentConfig.success || 
                                         currentConfig.autoConnect !== configData.autoConnect;

            // Se non ci sono modifiche, non salvare
            if (!hasSSIDChanges && !hasPasswordChanges && !hasAutoConnectChanges) {
                console.log('ℹ️ No significant changes detected');
                return {
                    success: true,
                    connecting: false,
                    message: 'No changes detected - configuration unchanged'
                };
            }

            // Prepara il payload
            const apiPayload = {
                primarySSID: configData.primarySSID,
                backupSSID: configData.backupSSID || '',
                autoConnect: configData.autoConnect !== false
            };

            // Gestione intelligente delle password
            const primaryPasswordProvided = configData.primaryPassword.trim() !== '';
            const primaryOpenNetwork = configData.primaryOpenNetwork;
            const backupPasswordProvided = configData.backupPassword.trim() !== '';
            const backupOpenNetwork = configData.backupOpenNetwork;

            if (primaryPasswordProvided) {
                // Password fornita: usa quella nuova
                apiPayload.primaryPassword = configData.primaryPassword;
                console.log('🔑 Using new primary password');
            } else if (primaryOpenNetwork) {
                // Rete aperta esplicitamente richiesta
                apiPayload.primaryPassword = '';
                apiPayload.forceEmptyPrimaryPassword = true;
                console.log('🔓 Setting primary network as OPEN (no password)');
            } else {
                // Password non fornita e non rete aperta: mantieni esistente se disponibile
                const hasExistingPrimaryConfig = currentConfig.success && 
                    currentConfig.configured_networks && 
                    currentConfig.configured_networks.some(n => n.is_current && n.ssid === configData.primarySSID);
                
                if (hasExistingPrimaryConfig) {
                    apiPayload.keepExistingPrimaryPassword = true;
                    console.log('🔒 Keeping existing primary password (if any)');
                } else {
                    // Nessuna configurazione esistente, imposta password vuota
                    apiPayload.primaryPassword = '';
                    console.log('🆕 New network - empty password');
                }
            }

            if (backupPasswordProvided) {
                // Password fornita: usa quella nuova
                apiPayload.backupPassword = configData.backupPassword;
                console.log('🔑 Using new backup password');
            } else if (backupOpenNetwork) {
                // Rete aperta esplicitamente richiesta
                apiPayload.backupPassword = '';
                apiPayload.forceEmptyBackupPassword = true;
                console.log('🔓 Setting backup network as OPEN (no password)');
            } else {
                // Password non fornita e non rete aperta: mantieni esistente se disponibile
                const hasExistingBackupConfig = currentConfig.success && 
                    currentConfig.configured_networks && 
                    currentConfig.configured_networks.some(n => !n.is_current && n.ssid === configData.backupSSID);
                
                if (hasExistingBackupConfig && configData.backupSSID.trim() !== '') {
                    apiPayload.keepExistingBackupPassword = true;
                    console.log('🔒 Keeping existing backup password (if any)');
                } else {
                    // Nessuna configurazione backup o SSID vuoto
                    apiPayload.backupPassword = '';
                    console.log('🆕 No backup or new backup - empty password');
                }
            }

            console.log('📤 Changes detected:', {
                ssidChanges: hasSSIDChanges,
                passwordChanges: hasPasswordChanges,
                autoConnectChanges: hasAutoConnectChanges
            });

            const result = await this.fetchJSON('/api/wifi/config', {
                method: 'POST',
                body: JSON.stringify(apiPayload)
            });

            console.log('✅ Configuration saved successfully');
            return {
                success: result.success || false,
                connecting: result.connecting || false,
                message: result.message || 'Configuration updated successfully'
            };
        } catch (error) {
            console.error('❌ Config save failed:', error);
            return {
                success: false,
                error: error.message
            };
        }
    }

    async resetConfig() {
        try {
            console.log('🗑️ Resetting WiFi configuration...');
            const result = await this.fetchJSON('/api/wifi/config', {
                method: 'DELETE'
            });

            console.log('✅ Configuration reset successfully');
            return {
                success: true,
                message: result.message || 'WiFi configuration reset'
            };
        } catch (error) {
            console.error('❌ Config reset failed:', error);
            return {
                success: false,
                error: error.message
            };
        }
    }

    async scanNetworks() {
        try {
            console.log('🔍 Scanning WiFi networks...');
            const data = await this.fetchJSON('/api/wifi/scan');
            
            const count = data.count || data.networks?.length || 0;
            console.log(`📡 Found ${count} networks`);
            
            return {
                success: true,
                networks: data.networks || [],
                count: count
            };
        } catch (error) {
            console.error('❌ Network scan failed:', error);
            return {
                success: false,
                error: error.message,
                networks: []
            };
        }
    }

    async getSystemStatus() {
        try {
            console.log('📊 Getting system status...');
            const status = await this.fetchJSON('/api/system/status');
            
            return {
                success: true,
                ...status,
                connected: status.wifi?.connected || false,
                uptime: status.system?.uptime || 0,
                heap_free: status.system?.heap || 0,
                system: {
                    uptime: status.system?.uptime || 0,
                    heap_free: status.system?.heap || 0,
                    heap_total: 320000,
                    cpu_freq: 240,
                    chipModel: status.system?.chipModel || 'ESP32',
                    flashSize: status.system?.flashSize || 0
                },
                wifi: {
                    status: status.wifi?.connected ? 'connected' : 'disconnected',
                    ssid: status.wifi?.ssid || '',
                    rssi: status.wifi?.rssi || -100,
                    ip: status.wifi?.ip || '',
                    mac: status.wifi?.mac || '',
                    quality: this.calculateSignalQuality(status.wifi?.rssi || -100)
                }
            };
        } catch (error) {
            console.error('❌ System status failed:', error);
            return {
                success: false,
                error: error.message
            };
        }
    }

    async testConnection(credentials) {
        try {
            console.log('🧪 Testing WiFi connection...', credentials.ssid);
            
            const testPayload = {
                ssid: credentials.ssid,
                password: credentials.password || ''
            };
            
            const result = await this.fetchJSON('/api/wifi/test', {
                method: 'POST',
                body: JSON.stringify(testPayload)
            });

            return {
                success: result.success || false,
                message: result.message || 'Test completed',
                ssid: result.ssid || credentials.ssid
            };
        } catch (error) {
            console.error('❌ Connection test failed:', error);
            return {
                success: false,
                error: error.message
            };
        }
    }

    calculateSignalQuality(rssi) {
        if (rssi >= -30) return 100;
        if (rssi >= -50) return 80;
        if (rssi >= -60) return 60;
        if (rssi >= -70) return 40;
        if (rssi >= -80) return 20;
        return 10;
    }

    validateConfig(config) {
        if (!config.primarySSID?.trim()) {
            throw new Error('Primary SSID is required');
        }
        if (config.primarySSID.length > 32) {
            throw new Error('SSID cannot exceed 32 characters');
        }
        if (config.primaryPassword && config.primaryPassword.length > 0) {
            if (config.primaryPassword.length < 8) {
                throw new Error('WiFi password must be at least 8 characters');
            }
            if (config.primaryPassword.length > 63) {
                throw new Error('WiFi password cannot exceed 63 characters');
            }
        }
    }
}

// ===============================
// UI MANAGER
// ===============================

class WiFiConfigUI {
    constructor() {
        this.api = new WiFiConfigAPI();
        this.isScanning = false;
        this.isSaving = false;
        this.networks = [];
        this.currentConfig = {};
        
        this.initializeEventListeners();
        this.loadInitialData();
    }

    initializeEventListeners() {
        document.getElementById('wifiForm')?.addEventListener('submit', async (e) => {
            e.preventDefault();
            await this.handleSaveConfig();
        });

        document.getElementById('scanBtn')?.addEventListener('click', () => this.handleScanNetworks());
        document.getElementById('resetBtn')?.addEventListener('click', () => this.handleResetConfig());
        document.getElementById('testBtn')?.addEventListener('click', () => this.handleTestConnection());
        document.getElementById('loadBtn')?.addEventListener('click', () => this.loadCurrentConfig());
        document.getElementById('refreshStatusBtn')?.addEventListener('click', () => this.refreshSystemStatus());

        document.getElementById('networkSelect')?.addEventListener('change', (e) => {
            this.handleSSIDSelection(e.target.value);
        });

        // Gestione checkbox "Open Network"
        document.getElementById('primaryOpenNetwork')?.addEventListener('change', (e) => {
            const passwordField = document.getElementById('primaryPassword');
            if (e.target.checked) {
                passwordField.disabled = true;
                passwordField.value = '';
                passwordField.placeholder = 'Open network - no password required';
            } else {
                passwordField.disabled = false;
                passwordField.placeholder = 'Enter WiFi password';
            }
        });

        document.getElementById('backupOpenNetwork')?.addEventListener('change', (e) => {
            const passwordField = document.getElementById('backupPassword');
            if (e.target.checked) {
                passwordField.disabled = true;
                passwordField.value = '';
                passwordField.placeholder = 'Open network - no password required';
            } else {
                passwordField.disabled = false;
                passwordField.placeholder = 'Enter backup WiFi password';
            }
        });
    }

    async loadInitialData() {
        try {
            this.showStatus('🔄 Loading initial data...', 'info');
            
            const [configResult, statusResult] = await Promise.all([
                this.loadCurrentConfig(),
                this.refreshSystemStatus()
            ]);

            if (configResult || statusResult) {
                this.showStatus('✅ Initial data loaded', 'success');
            } else {
                this.showStatus('⚠️ Could not load data', 'warning');
            }
        } catch (error) {
            console.error('Error loading initial data:', error);
            this.showStatus(`❌ Failed to load: ${error.message}`, 'error');
        }
    }

    async loadCurrentConfig() {
        try {
            const result = await this.api.loadConfig();
            
            if (result.success) {
                this.currentConfig = result;
                this.populateConfigForm(result);
                this.updateConnectionStatus(result.wifi);
                this.displayConfiguredNetworks(result.configured_networks);
                return true;
            } else {
                this.showStatus(`❌ Load failed: ${result.error}`, 'error');
                return false;
            }
        } catch (error) {
            this.showStatus(`❌ Load error: ${error.message}`, 'error');
            return false;
        }
    }

    async handleSaveConfig() {
        if (this.isSaving) return;
        
        try {
            this.isSaving = true;
            this.updateSaveButton(true);
            
            const formData = this.getFormData();
            const result = await this.api.saveConfig(formData);
            
            if (result.success) {
                this.showStatus('✅ Configuration saved successfully!', 'success');
                
                if (result.connecting) {
                    this.showStatus('🔄 Attempting to connect...', 'info');
                    setTimeout(() => this.pollConnectionStatus(), 3000);
                }
            } else {
                this.showStatus(`❌ Save failed: ${result.error}`, 'error');
            }
        } catch (error) {
            this.showStatus(`❌ Save error: ${error.message}`, 'error');
        } finally {
            this.isSaving = false;
            this.updateSaveButton(false);
        }
    }

    async handleResetConfig() {
        if (!confirm('⚠️ This will delete all WiFi configurations. Continue?')) {
            return;
        }

        try {
            this.showStatus('🗑️ Resetting configuration...', 'info');
            const result = await this.api.resetConfig();
            
            if (result.success) {
                this.showStatus('✅ Configuration reset successfully', 'success');
                this.clearConfigForm();
                this.currentConfig = {};
                
                const configElement = document.getElementById('configuredNetworks');
                if (configElement) {
                    configElement.innerHTML = '<div class="no-networks">No configured networks</div>';
                }
            } else {
                this.showStatus(`❌ Reset failed: ${result.error}`, 'error');
            }
        } catch (error) {
            this.showStatus(`❌ Reset error: ${error.message}`, 'error');
        }
    }

    async handleScanNetworks() {
        if (this.isScanning) return;
        
        try {
            this.isScanning = true;
            this.updateScanButton(true);
            this.showStatus('🔍 Scanning networks...', 'info');
            
            const result = await this.api.scanNetworks();
            
            if (result.success) {
                this.networks = result.networks;
                this.populateNetworkList(result.networks);
                this.showStatus(`📡 Found ${result.count} networks`, 'success');
            } else {
                this.showStatus(`❌ Scan failed: ${result.error}`, 'error');
            }
        } catch (error) {
            this.showStatus(`❌ Scan error: ${error.message}`, 'error');
        } finally {
            this.isScanning = false;
            this.updateScanButton(false);
        }
    }

    async handleTestConnection() {
        try {
            this.showStatus('🧪 Testing connection...', 'info');
            
            const formData = this.getFormData();
            if (!formData.primarySSID) {
                throw new Error('SSID is required for connection test');
            }
            
            const result = await this.api.testConnection({
                ssid: formData.primarySSID,
                password: formData.primaryPassword
            });
            
            if (result.success) {
                this.showStatus('✅ Connection test successful', 'success');
            } else {
                this.showStatus(`❌ Connection test failed: ${result.error}`, 'error');
            }
        } catch (error) {
            this.showStatus(`❌ Test error: ${error.message}`, 'error');
        }
    }

    async refreshSystemStatus() {
        try {
            const result = await this.api.getSystemStatus();
            
            if (result.success) {
                this.updateSystemStatusDisplay(result);
                this.updateConnectionStatus(result.wifi);
                return true;
            } else {
                console.error('System status failed:', result.error);
                return false;
            }
        } catch (error) {
            console.error('System status error:', error);
            return false;
        }
    }

    async pollConnectionStatus() {
        const maxAttempts = 15;
        let attempts = 0;
        
        const poll = async () => {
            if (attempts >= maxAttempts) {
                this.showStatus('⏰ Connection timeout', 'warning');
                return;
            }
            
            attempts++;
            const result = await this.api.getSystemStatus();
            
            if (result.success && result.wifi) {
                if (result.wifi.status === 'connected') {
                    this.showStatus(`✅ Connected to ${result.wifi.ssid}!`, 'success');
                    this.updateConnectionStatus(result.wifi);
                    await this.loadCurrentConfig();
                    return;
                } else if (attempts < maxAttempts) {
                    setTimeout(poll, 2000);
                    return;
                }
            }
            
            this.showStatus('⚠️ Connection failed', 'warning');
        };
        
        await poll();
    }

    // ===============================
    // UI UPDATES
    // ===============================
    
    populateConfigForm(config) {
        document.getElementById('primarySSID').value = config.primarySSID || '';
        document.getElementById('primaryPassword').value = '';
        document.getElementById('primaryOpenNetwork').checked = false;
        document.getElementById('backupSSID').value = config.backupSSID || '';
        document.getElementById('backupPassword').value = '';
        document.getElementById('backupOpenNetwork').checked = false;
        document.getElementById('autoConnect').checked = config.autoConnect !== false;
    }

    populateNetworkList(networks) {
        const select = document.getElementById('networkSelect');
        if (!select) return;
        
        select.innerHTML = '<option value="">📡 Select a network...</option>';
        
        const sortedNetworks = networks
            .filter(net => net.ssid && net.ssid.trim().length > 0)
            .sort((a, b) => (b.rssi || -100) - (a.rssi || -100));
        
        sortedNetworks.forEach(network => {
            const option = document.createElement('option');
            option.value = network.ssid;
            
            const signalIcon = network.rssi >= -50 ? '📶' : 
                              network.rssi >= -70 ? '📶' : '📶';
            const securityIcon = (network.encryption === 'open') ? '🔓' : '🔐';
            const channelInfo = network.channel ? ` Ch${network.channel}` : '';
            
            option.textContent = `${signalIcon} ${network.ssid} (${network.rssi}dBm${channelInfo}) ${securityIcon}`;
            select.appendChild(option);
        });
    }

    displayConfiguredNetworks(networks) {
        const element = document.getElementById('configuredNetworks');
        if (!element) return;
        
        if (!networks || networks.length === 0) {
            element.innerHTML = '<div class="no-networks">No configured networks</div>';
            return;
        }
        
        const html = networks.map(network => `
            <div class="network-item ${network.is_current ? 'current' : ''}">
                <div class="network-name">
                    <strong>${network.ssid}</strong>
                    ${network.is_current ? '<span class="current-badge">🟢 ACTIVE</span>' : ''}
                    ${network.is_hidden ? '<span class="hidden-badge">👁️ HIDDEN</span>' : ''}
                    ${network.is_preferred ? '<span class="preferred-badge">⭐ PREFERRED</span>' : ''}
                </div>
                <div class="network-details">
                    Signal: ${network.last_rssi}dBm | 
                    Fails: ${network.fail_count} |
                    Status: ${network.is_current ? 'Connected' : 'Configured'}
                </div>
            </div>
        `).join('');
        
        element.innerHTML = html;
    }

    updateConnectionStatus(wifiStatus) {
        const statusElement = document.getElementById('connectionStatus');
        if (!statusElement || !wifiStatus) return;
        
        if (wifiStatus.status === 'connected') {
            statusElement.innerHTML = `
                <div class="connection-status connected">
                    <strong>✅ Connected</strong><br>
                    Network: <strong>${wifiStatus.ssid}</strong><br>
                    Signal: ${wifiStatus.rssi}dBm (${wifiStatus.quality}%)<br>
                    IP Address: ${wifiStatus.ip}<br>
                    MAC Address: ${wifiStatus.mac || 'N/A'}
                </div>
            `;
        } else {
            statusElement.innerHTML = `
                <div class="connection-status disconnected">
                    <strong>❌ Disconnected</strong><br>
                    No active WiFi connection
                </div>
            `;
        }
    }

    updateSystemStatusDisplay(status) {
        const element = document.getElementById('systemStatus');
        if (!element) return;
        
        const uptimeHours = Math.floor((status.uptime || 0) / (1000 * 60 * 60));
        const uptimeMinutes = Math.floor(((status.uptime || 0) % (1000 * 60 * 60)) / (1000 * 60));
        
        const heapFree = status.system?.heap || status.heap_free || 0;
        const heapTotal = status.system?.heap_total || 320000;
        const heapUsed = heapTotal - heapFree;
        const heapUsagePercent = Math.round((heapUsed / heapTotal) * 100);
        
        const flashSizeMB = status.system?.flashSize ? Math.round(status.system.flashSize / 1024 / 1024) : null;
        
        element.innerHTML = `
            <div class="system-info">
                <div class="info-item">
                    <span class="label">🔧 Chip:</span>
                    <span class="value">${status.system?.chipModel || 'ESP32'}</span>
                </div>
                <div class="info-item">
                    <span class="label">⏱️ Uptime:</span>
                    <span class="value">${uptimeHours}h ${uptimeMinutes}m</span>
                </div>
                <div class="info-item">
                    <span class="label">💾 Memory:</span>
                    <span class="value">${heapUsagePercent}% used</span>
                </div>
                <div class="info-item">
                    <span class="label">🆓 Free Heap:</span>
                    <span class="value">${Math.round(heapFree / 1024)}KB</span>
                </div>
                ${flashSizeMB ? `
                <div class="info-item">
                    <span class="label">💽 Flash:</span>
                    <span class="value">${flashSizeMB}MB</span>
                </div>
                ` : ''}
            </div>
        `;
    }

    // ===============================
    // UTILITIES
    // ===============================
    
    getFormData() {
        return {
            primarySSID: document.getElementById('primarySSID')?.value.trim() || '',
            primaryPassword: document.getElementById('primaryPassword')?.value || '',
            primaryOpenNetwork: document.getElementById('primaryOpenNetwork')?.checked || false,
            backupSSID: document.getElementById('backupSSID')?.value.trim() || '',
            backupPassword: document.getElementById('backupPassword')?.value || '',
            backupOpenNetwork: document.getElementById('backupOpenNetwork')?.checked || false,
            autoConnect: document.getElementById('autoConnect')?.checked !== false
        };
    }

    showStatus(message, type) {
        const statusElement = document.getElementById('statusMessage');
        if (!statusElement) {
            console.log(`Status [${type}]: ${message}`);
            return;
        }
        
        statusElement.textContent = message;
        statusElement.className = `status ${type}`;
        statusElement.style.display = 'block';
        
        if (type === 'success' || type === 'error') {
            setTimeout(() => {
                if (statusElement.textContent === message) {
                    statusElement.style.display = 'none';
                }
            }, 5000);
        }
    }

    updateSaveButton(loading) {
        const button = document.getElementById('saveBtn');
        if (!button) return;
        
        if (loading) {
            button.innerHTML = '⏳ Saving...';
            button.disabled = true;
        } else {
            button.innerHTML = '💾 Save Configuration';
            button.disabled = false;
        }
    }

    updateScanButton(scanning) {
        const button = document.getElementById('scanBtn');
        if (!button) return;
        
        if (scanning) {
            button.innerHTML = '🔍 Scanning...';
            button.disabled = true;
        } else {
            button.innerHTML = '📡 Scan Networks';
            button.disabled = false;
        }
    }

    clearConfigForm() {
        const form = document.getElementById('wifiForm');
        if (form) {
            form.reset();
        }
    }

    handleSSIDSelection(ssid) {
        if (ssid && ssid.trim()) {
            const primarySSID = document.getElementById('primarySSID');
            const primaryPassword = document.getElementById('primaryPassword');
            
            if (primarySSID) primarySSID.value = ssid;
            if (primaryPassword) primaryPassword.focus();
        }
    }
}

// ===============================
// AUTO-INITIALIZATION
// ===============================

document.addEventListener('DOMContentLoaded', () => {
    window.wifiConfig = new WiFiConfigUI();
    console.log('✅ WiFi Configuration initialized');
});

if (typeof module !== 'undefined' && module.exports) {
    module.exports = { WiFiConfigAPI, WiFiConfigUI };
}
