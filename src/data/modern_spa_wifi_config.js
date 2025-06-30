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
            
            // Formato corretto per il tuo handler C++
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

    async rebootDevice() {
        try {
            console.log('🔄 Sending reboot command...');
            
            const result = await this.fetchJSON('/api/system/reboot', {
                method: 'POST'
            });

            return {
                success: result.success || true,
                message: result.message || 'Reboot command sent'
            };
        } catch (error) {
            // È normale che fallisca - l'ESP32 si disconnette durante il reboot
            console.log('Reboot request sent (connection lost as expected)');
            return {
                success: true,
                message: 'Reboot command sent'
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
        this.isRebooting = false; // Flag per prevenire doppi reboot
        this.networks = [];
        this.currentConfig = {};
        
        this.initializeEventListeners();
        this.loadInitialData();
    }

    initializeEventListeners() {
        console.log('🔗 Initializing event listeners...');
        
        document.getElementById('wifiForm')?.addEventListener('submit', async (e) => {
            e.preventDefault();
            await this.handleSaveConfig();
        });

        document.getElementById('scanBtn')?.addEventListener('click', () => this.handleScanNetworks());
        document.getElementById('resetBtn')?.addEventListener('click', () => this.handleResetConfig());
        document.getElementById('testBtn')?.addEventListener('click', () => this.handleTestConnection());
        
        // REBOOT BUTTON - Con debug e gestione tooltip
        const rebootBtn = document.getElementById('rebootBtn');
        if (rebootBtn) {
            console.log('✅ Reboot button found - attaching listener');
            rebootBtn.addEventListener('click', (e) => {
                // Previeni comportamenti default
                e.preventDefault();
                e.stopPropagation();
                
                console.log('🔄 Reboot button clicked!');
                this.handleReboot();
            });
            
            // Gestisci anche il mouse leave per nascondere tooltip
            rebootBtn.addEventListener('mouseleave', () => {
                this.hideAllTooltips();
            });
        } else {
            console.error('❌ Reboot button NOT found!');
        }
        
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

        // DEBUG TOGGLE per Progress Bar
        document.getElementById('debugProgressBar')?.addEventListener('change', (e) => {
            this.toggleProgressBarDebug(e.target.checked);
        });
        
        console.log('✅ Event listeners initialized');
    }

    toggleProgressBarDebug(show) {
        const progressContainer = document.getElementById('reconnectProgress');
        const progressBar = document.getElementById('progressBar');
        const progressText = document.getElementById('progressText');
        
        if (show) {
            // Mostra progress bar per debug
            if (progressContainer) progressContainer.style.display = 'block';
            if (progressBar) {
                progressBar.style.width = '60%';
                progressBar.textContent = '60%';
            }
            if (progressText) {
                progressText.textContent = 'DEBUG MODE: Progress bar visible for testing';
            }
            console.log('🔧 Debug progress bar enabled');
        } else {
            // Nascondi progress bar solo se non c'è un reboot in corso
            const isRebooting = progressText && progressText.textContent.includes('rebooting');
            if (!isRebooting && progressContainer) {
                progressContainer.style.display = 'none';
                console.log('🔧 Debug progress bar disabled');
            } else if (isRebooting) {
                console.log('🔧 Debug toggle ignored - reboot in progress');
            }
        }
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
                // Se fallisce tutto, almeno mostra l'IP corrente dalla URL
                this.updateIPFromURL();
            }
        } catch (error) {
            console.error('Error loading initial data:', error);
            this.showStatus(`❌ Failed to load: ${error.message}`, 'error');
            this.updateIPFromURL();
        }
    }

    updateIPFromURL() {
        // Fallback: estrae IP dalla URL corrente
        const ipElement = document.getElementById('currentIP');
        if (ipElement) {
            const currentHost = window.location.hostname;
            if (currentHost && currentHost !== 'localhost') {
                ipElement.textContent = currentHost;
                ipElement.style.color = '#ffc107'; // Giallo = da URL
            } else {
                ipElement.textContent = 'Unknown';
                ipElement.style.color = '#6c757d'; // Grigio
            }
        }
    }

    async loadCurrentConfig() {
        try {
            const result = await this.api.loadConfig();
            
            if (result.success) {
                this.currentConfig = result;
                this.populateConfigForm(result);
                this.updateConnectionStatus(result.wifi);
                this.updateIPDisplay(result.wifi);
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

    // VERSIONE CORRETTA della funzione handleSaveConfig nel JavaScript
	async handleSaveConfig() {
		if (this.isSaving) return;
		
		try {
			this.isSaving = true;
			this.updateSaveButton(true);
			
			const formData = this.getFormData();
			
			// ⭐ NOVITÀ: Rileva PRIMA se ci saranno modifiche
			const willNeedReconnection = this.doesConfigNeedReconnection(formData);
			console.log('🔍 Pre-save analysis:', {
				willNeedReconnection: willNeedReconnection,
				currentSSID: this.currentConfig?.primarySSID || 'unknown',
				newSSID: formData.primarySSID,
				hasNewPassword: formData.primaryPassword?.length > 0,
				isOpenNetwork: formData.primaryOpenNetwork
			});
			
			const result = await this.api.saveConfig(formData);
			
			if (result.success) {
				// ⭐ LOGICA SEMPLIFICATA: Se abbiamo modifiche WiFi, sempre progress bar
				const needsProgress = result.reconnecting || 
									result.connecting || 
									willNeedReconnection ||
									this.hasWiFiChanges(formData);
				
				console.log('📊 Post-save analysis:', {
					serverSaysReconnecting: result.reconnecting,
					serverSaysConnecting: result.connecting,
					localAnalysis: willNeedReconnection,
					hasWiFiChanges: this.hasWiFiChanges(formData),
					finalDecision: needsProgress
				});
				
				if (needsProgress) {
					console.log('🚀 Starting progress bar countdown');
					this.showStatus('✅ Configuration saved - reconnecting...', 'success');
					this.startReconnectionCountdown();
				} else {
					console.log('ℹ️ No progress bar needed - minor changes only');
					this.showStatus('✅ Configuration saved successfully!', 'success');
					// Ricarica comunque i dati per aggiornare l'UI
					setTimeout(() => this.loadCurrentConfig(), 1000);
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

	// ⭐ NUOVA FUNZIONE: Rileva se ci sono modifiche WiFi significative
	hasWiFiChanges(newConfig) {
		// Se l'SSID primario è diverso dal form, è una modifica
		if (newConfig.primarySSID && newConfig.primarySSID.trim() !== '') {
			const currentPrimarySSID = this.currentConfig?.primarySSID || '';
			if (newConfig.primarySSID !== currentPrimarySSID) {
				console.log('🔄 SSID change detected:', {
					old: currentPrimarySSID,
					new: newConfig.primarySSID
				});
				return true;
			}
		}
		
		// Se è stata fornita una password, è una modifica
		if (newConfig.primaryPassword && newConfig.primaryPassword.trim() !== '') {
			console.log('🔄 New password provided');
			return true;
		}
		
		// Se è cambiato lo stato "open network", è una modifica
		if (newConfig.primaryOpenNetwork) {
			console.log('🔄 Open network mode selected');
			return true;
		}
		
		// Se l'SSID backup è diverso, è una modifica (meno critica ma comunque)
		if (newConfig.backupSSID && newConfig.backupSSID.trim() !== '') {
			const currentBackupSSID = this.currentConfig?.backupSSID || '';
			if (newConfig.backupSSID !== currentBackupSSID) {
				console.log('🔄 Backup SSID change detected');
				return true;
			}
		}
		
		return false;
	}

	// ⭐ VERSIONE MIGLIORATA della funzione esistente
	doesConfigNeedReconnection(newConfig) {
		// Se non abbiamo configurazione precedente, assume riconnessione necessaria
		if (!this.currentConfig || !this.currentConfig.success) {
			console.log('🔄 No previous config - reconnection needed');
			return true;
		}
		
		// Verifica se l'SSID primario è cambiato
		if (this.currentConfig.primarySSID !== newConfig.primarySSID) {
			console.log('🔄 SSID change detected - reconnection needed');
			return true;
		}
		
		// Verifica se è stata fornita una nuova password
		if (newConfig.primaryPassword && newConfig.primaryPassword.trim() !== '') {
			console.log('🔄 New password provided - reconnection needed');
			return true;
		}
		
		// Verifica se è cambiato lo stato "open network"
		if (newConfig.primaryOpenNetwork) {
			console.log('🔄 Open network mode change - reconnection needed');
			return true;
		}
		
		// Verifica se è cambiato l'auto-connect
		if (this.currentConfig.autoConnect !== newConfig.autoConnect) {
			console.log('🔄 Auto-connect setting changed');
			return false; // Non richiede riconnessione immediata
		}
		
		return false;
	}

    startReconnectionCountdown() {
        let countdown = 10;
        
        // Nascondi status normale e mostra progress
        const statusMessage = document.getElementById('statusMessage');
        const progressContainer = document.getElementById('reconnectProgress');
        const progressBar = document.getElementById('progressBar');
        const progressText = document.getElementById('progressText');
        const debugToggle = document.getElementById('debugProgressBar');
        
        // Nascondi messaggio status normale
        if (statusMessage) statusMessage.style.display = 'none';
        
        // Mostra progress container (anche se debug è attivo)
        if (progressContainer) progressContainer.style.display = 'block';
        
        // Se il debug è attivo, disattivalo automaticamente
        if (debugToggle && debugToggle.checked) {
            debugToggle.checked = false;
            console.log('🔧 Debug mode disabled - real countdown starting');
        }
        
        const updateCountdown = () => {
            if (countdown > 0) {
                const percentage = ((10 - countdown) / 10) * 100;
                
                // Aggiorna progress bar
                if (progressBar) {
                    progressBar.style.width = percentage + '%';
                    progressBar.textContent = `${Math.round(percentage)}%`;
                }
                
                // Aggiorna testo
                if (progressText) {
                    progressText.textContent = `REAL COUNTDOWN: Page will reload in ${countdown} seconds...`;
                }
                
                console.log(`🔄 Countdown: ${countdown}s (${Math.round(percentage)}%)`);
                
                countdown--;
                setTimeout(updateCountdown, 1000);
            } else {
                // Finito - 100%
                if (progressBar) {
                    progressBar.style.width = '100%';
                    progressBar.textContent = 'Complete!';
                }
                if (progressText) {
                    progressText.textContent = 'Reloading page now...';
                }
                
                console.log('🔄 Countdown completed - reloading page');
                setTimeout(() => {
                    window.location.reload();
                }, 500);
            }
        };
        
        updateCountdown();
    }

    async handleReboot() {
        // Nascondi eventuali tooltip o popup prima di mostrare il confirm
        this.hideAllTooltips();
        
        // Previeni doppi click
        if (this.isRebooting) {
            console.log('🔄 Reboot already in progress, ignoring...');
            return;
        }
        
        if (!confirm('⚠️ This will restart the ESP32 device. Continue?')) {
            return;
        }

        try {
            this.isRebooting = true; // Flag per prevenire doppi click
            this.showStatus('🔄 Rebooting ESP32...', 'info');
            
            const result = await this.api.rebootDevice();
            
            if (result.success) {
                // Avvia countdown per riconnessione
                this.startRebootCountdown();
            } else {
                this.showStatus(`❌ Reboot failed: ${result.error}`, 'error');
                this.isRebooting = false; // Reset flag se fallisce
            }
        } catch (error) {
            // È normale che fallisca - l'ESP32 si sta riavviando
            console.log('Expected error during reboot:', error);
            this.startRebootCountdown();
        }
    }

    hideAllTooltips() {
        // Rimuovi tutti i tooltip attivi
        const tooltips = document.querySelectorAll('[title]');
        tooltips.forEach(el => {
            el.setAttribute('data-original-title', el.getAttribute('title') || '');
            el.removeAttribute('title');
        });
        
        // Forza la chiusura di eventuali tooltip Bootstrap o simili
        const activeTooltips = document.querySelectorAll('.tooltip, .bs-tooltip-top, .bs-tooltip-bottom, .bs-tooltip-left, .bs-tooltip-right');
        activeTooltips.forEach(tooltip => {
            tooltip.remove();
        });
        
        // Rimuovi anche eventuali overlay
        const overlays = document.querySelectorAll('.tooltip-backdrop, .modal-backdrop');
        overlays.forEach(overlay => {
            overlay.remove();
        });
    }

    startRebootCountdown() {
        let countdown = 15; // Aumentato a 15 secondi per dare tempo all'ESP32
        
        const statusMessage = document.getElementById('statusMessage');
        const progressContainer = document.getElementById('reconnectProgress');
        const progressBar = document.getElementById('progressBar');
        const progressText = document.getElementById('progressText');
        const debugToggle = document.getElementById('debugProgressBar');
        
        // Nascondi messaggio status normale
        if (statusMessage) statusMessage.style.display = 'none';
        
        // Mostra progress container
        if (progressContainer) progressContainer.style.display = 'block';
        
        // Se il debug è attivo, disattivalo automaticamente
        if (debugToggle && debugToggle.checked) {
            debugToggle.checked = false;
            console.log('🔧 Debug mode disabled - reboot countdown starting');
        }
        
        const updateCountdown = () => {
            if (countdown > 0) {
                const percentage = ((15 - countdown) / 15) * 100;
                
                // Aggiorna progress bar
                if (progressBar) {
                    progressBar.style.width = percentage + '%';
                    progressBar.textContent = `${Math.round(percentage)}%`;
                }
                
                // Aggiorna testo
                if (progressText) {
                    progressText.textContent = `ESP32 rebooting... Reconnecting in ${countdown} seconds...`;
                }
                
                console.log(`🔄 Reboot countdown: ${countdown}s (${Math.round(percentage)}%)`);
                countdown--;
                setTimeout(updateCountdown, 1000);
            } else {
                // Finito - completa la barra e ricarica immediatamente
                if (progressBar) {
                    progressBar.style.width = '100%';
                    progressBar.textContent = '100%';
                }
                if (progressText) {
                    progressText.textContent = 'Reconnecting now...';
                }
                
                console.log('🔄 Reboot countdown completed - reloading immediately');
                
                // Ricarica immediatamente senza attesa extra
                setTimeout(() => {
                    if (progressContainer) {
                        progressContainer.style.display = 'none';
                    }
                    window.location.reload();
                }, 500); // Solo 500ms per vedere il 100%
            }
        };
        
        updateCountdown();
    }

    async handleResetConfig() {
        // Nascondi eventuali tooltip prima del confirm
        this.hideAllTooltips();
        
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
                this.updateIPDisplay(result.wifi); // Aggiorna IP nel header
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

    updateIPDisplay(wifiStatus) {
        const ipElement = document.getElementById('currentIP');
        if (!ipElement) return;
        
        if (wifiStatus && wifiStatus.ip) {
            ipElement.textContent = wifiStatus.ip;
            ipElement.style.color = '#28a745'; // Verde se connesso
        } else {
            ipElement.textContent = 'Not connected';
            ipElement.style.color = '#dc3545'; // Rosso se disconnesso
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
            primarySSID: document.getElementById('primarySSID')?.value?.trim() || '',
            primaryPassword: document.getElementById('primaryPassword')?.value || '',
            primaryOpenNetwork: document.getElementById('primaryOpenNetwork')?.checked || false,
            backupSSID: document.getElementById('backupSSID')?.value?.trim() || '',
            backupPassword: document.getElementById('backupPassword')?.value || '',
            backupOpenNetwork: document.getElementById('backupOpenNetwork')?.checked || false,
            autoConnect: document.getElementById('autoConnect')?.checked !== false
        };
    }

    showStatus(message, type) {
        const statusElement = document.getElementById('statusMessage');
        const progressContainer = document.getElementById('reconnectProgress');
        
        if (!statusElement) {
            console.log(`Status [${type}]: ${message}`);
            return;
        }
        
        // Nascondi progress se mostriamo status normale (ma solo se non è un reboot)
        if (progressContainer && !message.includes('Rebooting')) {
            progressContainer.style.display = 'none';
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