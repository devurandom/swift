/*
 * Copyright (c) 2015-2016 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#include <Swiften/TLS/SecureTransport/SecureTransportContext.h>

#include <boost/type_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <Swiften/Base/Algorithm.h>
#include <Swiften/Base/Log.h>
#include <Swiften/TLS/SecureTransport/SecureTransportCertificate.h>
#include <Swiften/TLS/PKCS12Certificate.h>
#include <Swiften/TLS/CertificateWithKey.h>

#include <Cocoa/Cocoa.h>

#import <Security/SecCertificate.h>
#import <Security/SecImportExport.h>

namespace {
    typedef boost::remove_pointer<CFArrayRef>::type CFArray;
    typedef boost::remove_pointer<SecTrustRef>::type SecTrust;
}

template <typename T, typename S>
T bridge_cast(S source) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
    return (__bridge T)(source);
#pragma clang diagnostic pop
}

namespace Swift {

namespace {


CFArrayRef CreateClientCertificateChainAsCFArrayRef(CertificateWithKey::ref key) {
    std::shared_ptr<PKCS12Certificate> pkcs12 = std::dynamic_pointer_cast<PKCS12Certificate>(key);
    if (!key) {
        return nullptr;
    }

    SafeByteArray safePassword = pkcs12->getPassword();
    CFIndex passwordSize = 0;
    try {
        passwordSize = boost::numeric_cast<CFIndex>(safePassword.size());
    } catch (...) {
        return nullptr;
    }

    CFMutableArrayRef certChain = CFArrayCreateMutable(nullptr, 0, nullptr);

    OSStatus securityError = errSecSuccess;
    CFStringRef password = CFStringCreateWithBytes(kCFAllocatorDefault, safePassword.data(), passwordSize, kCFStringEncodingUTF8, false);
    const void* keys[] = { kSecImportExportPassphrase };
    const void* values[] = { password };

    CFDictionaryRef options = CFDictionaryCreate(nullptr, keys, values, 1, nullptr, nullptr);

    CFArrayRef items = nullptr;
    CFDataRef pkcs12Data = bridge_cast<CFDataRef>([NSData dataWithBytes: static_cast<const void *>(pkcs12->getData().data()) length:pkcs12->getData().size()]);
    securityError = SecPKCS12Import(pkcs12Data, options, &items);
    CFRelease(options);
    NSArray* nsItems = bridge_cast<NSArray*>(items);

    switch(securityError) {
        case errSecSuccess:
            break;
        case errSecAuthFailed:
            // Password did not work for decoding the certificate.
            SWIFT_LOG(warning) << "Invalid password." << std::endl;
            break;
        case errSecDecode:
            // Other decoding error.
            SWIFT_LOG(warning) << "PKCS12 decoding error." << std::endl;
            break;
        default:
            SWIFT_LOG(warning) << "Unknown error." << std::endl;
    }

    if (securityError != errSecSuccess) {
        if (items) {
            CFRelease(items);
            items = nullptr;
        }
        CFRelease(certChain);
        certChain = nullptr;
    }

    if (certChain) {
        CFArrayAppendValue(certChain, nsItems[0][@"identity"]);

        for (CFIndex index = 0; index < CFArrayGetCount(bridge_cast<CFArrayRef>(nsItems[0][@"chain"])); index++) {
            CFArrayAppendValue(certChain, CFArrayGetValueAtIndex(bridge_cast<CFArrayRef>(nsItems[0][@"chain"]), index));
        }
    }
    return certChain;
}

}

SecureTransportContext::SecureTransportContext(bool checkCertificateRevocation) : state_(None), checkCertificateRevocation_(checkCertificateRevocation) {
    sslContext_ = std::shared_ptr<SSLContext>(SSLCreateContext(nullptr, kSSLClientSide, kSSLStreamType), CFRelease);

    OSStatus error = noErr;
    // set IO callbacks
    error = SSLSetIOFuncs(sslContext_.get(), &SecureTransportContext::SSLSocketReadCallback, &SecureTransportContext::SSLSocketWriteCallback);
    if (error != noErr) {
        SWIFT_LOG(error) << "Unable to set IO functions to SSL context." << std::endl;
        sslContext_.reset();
    }

    error = SSLSetConnection(sslContext_.get(), this);
    if (error != noErr) {
        SWIFT_LOG(error) << "Unable to set connection to SSL context." << std::endl;
        sslContext_.reset();
    }


    error = SSLSetSessionOption(sslContext_.get(), kSSLSessionOptionBreakOnServerAuth, true);
    if (error != noErr) {
        SWIFT_LOG(error) << "Unable to set kSSLSessionOptionBreakOnServerAuth on session." << std::endl;
        sslContext_.reset();
    }
}

SecureTransportContext::~SecureTransportContext() {
    if (sslContext_) {
        SSLClose(sslContext_.get());
    }
}

std::string SecureTransportContext::stateToString(State state) {
    std::string returnValue;
    switch(state) {
        case Handshake:
            returnValue = "Handshake";
            break;
        case HandshakeDone:
            returnValue = "HandshakeDone";
            break;
        case None:
            returnValue = "None";
            break;
        case Error:
            returnValue = "Error";
            break;
    }
    return returnValue;
}

void SecureTransportContext::setState(State newState) {
    SWIFT_LOG(debug) << "Switch state from " << stateToString(state_) << " to " << stateToString(newState) << "." << std::endl;
    state_ = newState;
}

void SecureTransportContext::connect() {
    SWIFT_LOG_ASSERT(state_ == None, error) << "current state '" << stateToString(state_) << " invalid." << std::endl;
    if (clientCertificate_) {
        CFArrayRef certs = CreateClientCertificateChainAsCFArrayRef(clientCertificate_);
        if (certs) {
            std::shared_ptr<CFArray> certRefs(certs, CFRelease);
            OSStatus result = SSLSetCertificate(sslContext_.get(), certRefs.get());
            if (result != noErr) {
                SWIFT_LOG(error) << "SSLSetCertificate failed with error " << result << "." << std::endl;
            }
        }
    }
    processHandshake();
}

void SecureTransportContext::processHandshake() {
    SWIFT_LOG_ASSERT(state_ == None || state_ == Handshake, error) << "current state '" << stateToString(state_) << " invalid." << std::endl;
    OSStatus error = SSLHandshake(sslContext_.get());
    if (error == errSSLWouldBlock) {
        setState(Handshake);
    }
    else if (error == noErr) {
        SWIFT_LOG(debug) << "TLS handshake successful." << std::endl;
        setState(HandshakeDone);
        onConnected();
    }
    else if (error == errSSLPeerAuthCompleted) {
        SWIFT_LOG(debug) << "Received server certificate. Start verification." << std::endl;
        setState(Handshake);
        verifyServerCertificate();
    }
    else {
        SWIFT_LOG(debug) << "Error returned from SSLHandshake call is " << error << "." << std::endl;
        fatalError(nativeToTLSError(error), std::make_shared<CertificateVerificationError>());
    }
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

void SecureTransportContext::verifyServerCertificate() {
    SecTrustRef trust = nullptr;
    OSStatus error = SSLCopyPeerTrust(sslContext_.get(), &trust);
    if (error != noErr) {
        fatalError(std::make_shared<TLSError>(), std::make_shared<CertificateVerificationError>());
        return;
    }
    std::shared_ptr<SecTrust> trustRef = std::shared_ptr<SecTrust>(trust, CFRelease);

    if (checkCertificateRevocation_) {
        error = SecTrustSetOptions(trust, kSecTrustOptionRequireRevPerCert | kSecTrustOptionFetchIssuerFromNet);
        if (error != noErr) {
            fatalError(std::make_shared<TLSError>(), std::make_shared<CertificateVerificationError>());
            return;
        }
    }

    SecTrustResultType trustResult;
    error = SecTrustEvaluate(trust, &trustResult);
    if (error != errSecSuccess) {
        fatalError(std::make_shared<TLSError>(), std::make_shared<CertificateVerificationError>());
        return;
    }

    OSStatus cssmResult = 0;
    switch(trustResult) {
        case kSecTrustResultUnspecified:
            SWIFT_LOG(warning) << "Successful implicit validation. Result unspecified." << std::endl;
            break;
        case kSecTrustResultProceed:
            SWIFT_LOG(warning) << "Validation resulted in explicitly trusted." << std::endl;
            break;
        case kSecTrustResultRecoverableTrustFailure:
            SWIFT_LOG(warning) << "recoverable trust failure" << std::endl;
            error = SecTrustGetCssmResultCode(trust, &cssmResult);
            if (error == errSecSuccess) {
                verificationError_ = CSSMErrorToVerificationError(cssmResult);
                if (cssmResult == CSSMERR_TP_VERIFY_ACTION_FAILED || cssmResult == CSSMERR_APPLETP_INCOMPLETE_REVOCATION_CHECK ) {
                    // Find out the reason why the verification failed.
                    CFArrayRef certChain;
                    CSSM_TP_APPLE_EVIDENCE_INFO* statusChain;
                    error = SecTrustGetResult(trustRef.get(), &trustResult, &certChain, &statusChain);
                    if (error == errSecSuccess) {
                        std::shared_ptr<CFArray> certChainRef = std::shared_ptr<CFArray>(certChain, CFRelease);
                        for (CFIndex index = 0; index < CFArrayGetCount(certChainRef.get()); index++) {
                            for (CFIndex n = 0; n < statusChain[index].NumStatusCodes; n++) {
                                // Even though Secure Transport reported CSSMERR_APPLETP_INCOMPLETE_REVOCATION_CHECK on the whole certificate
                                // chain, the actual cause can be that a revocation check for a specific cert returned CSSMERR_TP_CERT_REVOKED.
                                if (!verificationError_ || verificationError_->getType() == CertificateVerificationError::RevocationCheckFailed) {
                                    verificationError_ = CSSMErrorToVerificationError(statusChain[index].StatusCodes[n]);
                                }
                            }
                        }
                    }
                    else {

                    }
                }
            }
            else {
                verificationError_ = std::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
            }
            break;
        case kSecTrustResultInvalid:
            verificationError_ = std::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
            break;
        case kSecTrustResultConfirm:
            // TODO: Confirmation from the user is required before proceeding.
            verificationError_ = std::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
            break;
        case kSecTrustResultDeny:
            // The user specified that the certificate should not be trusted.
            verificationError_ =  std::make_shared<CertificateVerificationError>(CertificateVerificationError::Untrusted);
            break;
        case kSecTrustResultFatalTrustFailure:
            // Trust denied; no simple fix is available.
            verificationError_ = std::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
            break;
        case kSecTrustResultOtherError:
            verificationError_ = std::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
            break;
    }

    // We proceed with the TLS handshake here to give the application an opportunity
    // to apply custom validation and trust management. The application is responsible
    // to call \ref getPeerCertificateVerificationError directly after the \ref onConnected
    // signal is called and before any application data is send to the context.
    processHandshake();
}

#pragma clang diagnostic pop

bool SecureTransportContext::setClientCertificate(CertificateWithKey::ref cert) {
    CFArrayRef nativeClientChain = CreateClientCertificateChainAsCFArrayRef(cert);
    if (nativeClientChain) {
        clientCertificate_ = cert;
        CFRelease(nativeClientChain);
        return true;
    }
    else {
        return false;
    }
}

void SecureTransportContext::handleDataFromNetwork(const SafeByteArray& data) {
    SWIFT_LOG(debug) << std::endl;
    SWIFT_LOG_ASSERT(state_ == HandshakeDone || state_ == Handshake, error) << "current state '" << stateToString(state_) << " invalid." << std::endl;

    append(readingBuffer_, data);

    size_t bytesRead = 0;
    OSStatus error = noErr;
    SafeByteArray applicationData;

    switch(state_) {
        case None:
            assert(false && "Invalid state 'None'.");
            break;
        case Handshake:
            processHandshake();
            break;
        case HandshakeDone:
            while (error == noErr) {
                applicationData.resize(readingBuffer_.size());
                error = SSLRead(sslContext_.get(), applicationData.data(), applicationData.size(), &bytesRead);
                if (error == noErr) {
                    // Read successful.
                }
                else if (error == errSSLWouldBlock) {
                    // Secure Transport does not want more data.
                    break;
                }
                else {
                    SWIFT_LOG(error) << "SSLRead failed with error " << error << ", read bytes: " << bytesRead << "." << std::endl;
                    fatalError(std::make_shared<TLSError>(), std::make_shared<CertificateVerificationError>());
                    return;
                }

                if (bytesRead > 0) {
                    applicationData.resize(bytesRead);
                    onDataForApplication(applicationData);
                }
                else {
                    break;
                }
            }
            break;
        case Error:
            SWIFT_LOG(debug) << "Igoring received data in error state." << std::endl;
            break;
    }
}


void SecureTransportContext::handleDataFromApplication(const SafeByteArray& data) {
    size_t processedBytes = 0;
    OSStatus error = SSLWrite(sslContext_.get(), data.data(), data.size(), &processedBytes);
    switch(error) {
        case errSSLWouldBlock:
            SWIFT_LOG(warning) << "Unexpected because the write callback does not block." << std::endl;
            return;
        case errSSLClosedGraceful:
        case noErr:
            return;
        default:
            SWIFT_LOG(warning) << "SSLWrite returned error code: " << error << ", processed bytes: " << processedBytes << std::endl;
            fatalError(std::make_shared<TLSError>(), std::shared_ptr<CertificateVerificationError>());
    }
}

std::vector<Certificate::ref> SecureTransportContext::getPeerCertificateChain() const {
    std::vector<Certificate::ref> peerCertificateChain;

    if (sslContext_) {
            typedef boost::remove_pointer<SecTrustRef>::type SecTrust;
            std::shared_ptr<SecTrust> securityTrust;

            SecTrustRef secTrust = nullptr;;
            OSStatus error = SSLCopyPeerTrust(sslContext_.get(), &secTrust);
            if (error == noErr) {
                securityTrust = std::shared_ptr<SecTrust>(secTrust, CFRelease);

                CFIndex chainSize = SecTrustGetCertificateCount(securityTrust.get());
                for (CFIndex n = 0; n < chainSize; n++) {
                    SecCertificateRef certificate = SecTrustGetCertificateAtIndex(securityTrust.get(), n);
                    if (certificate) {
                        peerCertificateChain.push_back(std::make_shared<SecureTransportCertificate>(certificate));
                    }
                }
            }
            else {
                SWIFT_LOG(warning) << "Failed to obtain peer trust structure; error = " << error << "." << std::endl;
            }
    }

    return peerCertificateChain;
}

CertificateVerificationError::ref SecureTransportContext::getPeerCertificateVerificationError() const {
    return verificationError_;
}

ByteArray SecureTransportContext::getFinishMessage() const {
    SWIFT_LOG(warning) << "Access to TLS handshake finish message is not part of OS X Secure Transport APIs." << std::endl;
    return ByteArray();
}

/**
 *    This I/O callback simulates an asynchronous read to the read buffer of the context. If it is empty, it returns errSSLWouldBlock; else
 *  the data within the buffer is returned.
 */
OSStatus SecureTransportContext::SSLSocketReadCallback(SSLConnectionRef connection, void *data, size_t *dataLength) {
    SecureTransportContext* context = const_cast<SecureTransportContext*>(static_cast<const SecureTransportContext*>(connection));
    OSStatus retValue = noErr;

    if (context->readingBuffer_.size() < *dataLength) {
        // Would block because Secure Transport is trying to read more data than there currently is available in the buffer.
        *dataLength = 0;
        retValue = errSSLWouldBlock;
    }
    else {
        size_t bufferLen = *dataLength;
        size_t copyToBuffer = bufferLen < context->readingBuffer_.size() ? bufferLen : context->readingBuffer_.size();

        memcpy(data, context->readingBuffer_.data(), copyToBuffer);

        context->readingBuffer_ = SafeByteArray(context->readingBuffer_.data() + copyToBuffer, context->readingBuffer_.data() + context->readingBuffer_.size());
        *dataLength = copyToBuffer;
    }
    return retValue;
}

OSStatus SecureTransportContext::SSLSocketWriteCallback(SSLConnectionRef connection, const void *data, size_t *dataLength) {
    SecureTransportContext* context = const_cast<SecureTransportContext*>(static_cast<const SecureTransportContext*>(connection));
    OSStatus retValue = noErr;

    SafeByteArray safeData;
    safeData.resize(*dataLength);
    memcpy(safeData.data(), data, safeData.size());

    context->onDataForNetwork(safeData);
    return retValue;
}

std::shared_ptr<TLSError> SecureTransportContext::nativeToTLSError(OSStatus /* error */) {
    std::shared_ptr<TLSError> swiftenError;
    swiftenError = std::make_shared<TLSError>();
    return swiftenError;
}

std::shared_ptr<CertificateVerificationError> SecureTransportContext::CSSMErrorToVerificationError(OSStatus resultCode) {
    std::shared_ptr<CertificateVerificationError> error;
    switch(resultCode) {
        case CSSMERR_TP_NOT_TRUSTED:
            SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_NOT_TRUSTED" << std::endl;
            error = std::make_shared<CertificateVerificationError>(CertificateVerificationError::Untrusted);
            break;
        case CSSMERR_TP_CERT_NOT_VALID_YET:
            SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_CERT_NOT_VALID_YET" << std::endl;
            error = std::make_shared<CertificateVerificationError>(CertificateVerificationError::NotYetValid);
            break;
        case CSSMERR_TP_CERT_EXPIRED:
            SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_CERT_EXPIRED" << std::endl;
            error = std::make_shared<CertificateVerificationError>(CertificateVerificationError::Expired);
            break;
        case CSSMERR_TP_CERT_REVOKED:
            SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_CERT_REVOKED" << std::endl;
            error = std::make_shared<CertificateVerificationError>(CertificateVerificationError::Revoked);
            break;
        case CSSMERR_TP_VERIFY_ACTION_FAILED:
            SWIFT_LOG(debug) << "CSSM result code: CSSMERR_TP_VERIFY_ACTION_FAILED" << std::endl;
            break;
        case CSSMERR_APPLETP_INCOMPLETE_REVOCATION_CHECK:
            SWIFT_LOG(debug) << "CSSM result code: CSSMERR_APPLETP_INCOMPLETE_REVOCATION_CHECK" << std::endl;
            if (checkCertificateRevocation_) {
                error = std::make_shared<CertificateVerificationError>(CertificateVerificationError::RevocationCheckFailed);
            }
            break;
        case CSSMERR_APPLETP_OCSP_UNAVAILABLE:
            SWIFT_LOG(debug) << "CSSM result code: CSSMERR_APPLETP_OCSP_UNAVAILABLE" << std::endl;
            if (checkCertificateRevocation_) {
                error = std::make_shared<CertificateVerificationError>(CertificateVerificationError::RevocationCheckFailed);
            }
            break;
        case CSSMERR_APPLETP_SSL_BAD_EXT_KEY_USE:
            SWIFT_LOG(debug) << "CSSM result code: CSSMERR_APPLETP_SSL_BAD_EXT_KEY_USE" << std::endl;
            error = std::make_shared<CertificateVerificationError>(CertificateVerificationError::InvalidPurpose);
            break;
        default:
            SWIFT_LOG(warning) << "unhandled CSSM error: " << resultCode << ", CSSM_TP_BASE_TP_ERROR: " << CSSM_TP_BASE_TP_ERROR << std::endl;
            error = std::make_shared<CertificateVerificationError>(CertificateVerificationError::UnknownError);
            break;
    }
    return error;
}

void SecureTransportContext::fatalError(std::shared_ptr<TLSError> error, std::shared_ptr<CertificateVerificationError> certificateError) {
    setState(Error);
    if (sslContext_) {
        SSLClose(sslContext_.get());
    }
    verificationError_ = certificateError;
    onError(error);
}

}
